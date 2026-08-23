#include "RenderingDevice.h"

#include <algorithm>
#include <ranges>
#include <spdlog/spdlog.h>

#include "RenderingContextDriver.h"
#include "RenderingDeviceDriver.h"
#include "error/CantCreateError.h"
#include "error/Macros.h"
#include "error/SwapchainError.h"

namespace Vixen {
    void RenderingDevice::waitForFrame(
        const uint32_t frameIndex
    ) {
        if (!frames[frameIndex].fenceSignaled)
            return;

        renderingDeviceDriver->waitOnFence(frames[frameIndex].fence).value();
        frames[frameIndex].fenceSignaled = false;
    }

    void RenderingDevice::waitForFrames() {
        for (uint32_t i = 0; i < frames.size(); i++)
            waitForFrame(i);
    }

    void RenderingDevice::flushAndWaitForFrames() {
        waitForFrames();
        endFrame();
        executeFrame(false);
        beginFrame(false);
    }

    void RenderingDevice::beginFrame(
        const bool presented
    ) {
        waitForFrame(frameIndex);

        if (!renderingDeviceDriver->resetCommandPool(frames[frameIndex].commandPool))
            throw std::runtime_error("Failed to reset command pool");
        if (!renderingDeviceDriver->beginCommandBuffer(frames[frameIndex].commandBuffer))
            throw std::runtime_error("Failed to begin command buffer");

        // TODO: Free this frame's resources
    }

    void RenderingDevice::endFrame() {
        renderingDeviceDriver->endCommandBuffer(frames[frameIndex].commandBuffer);
    }

    void RenderingDevice::executeChainedCommands(
        const bool present,
        Fence* drawFence,
        Semaphore* drawSemaphoreToSignal
    ) {
        if (!renderingDeviceDriver->executeCommandQueueAndPresent(
            graphicsQueue,
            frames[frameIndex].waitSemaphores,
            frames[frameIndex].commandBuffer
                ? std::vector{frames[frameIndex].commandBuffer}
                : std::vector<CommandBuffer*>{},
            drawSemaphoreToSignal
                ? std::vector{drawSemaphoreToSignal}
                : std::vector<Semaphore*>{},
            drawFence,
            present
                ? frames[frameIndex].swapchainsToPresent
                : std::vector<Swapchain*>{}
        ))
            throw std::runtime_error("Failed to execute chained commands");

        frames[frameIndex].waitSemaphores.clear();
    }

    void RenderingDevice::executeFrame(
        const bool present
    ) {
        const bool canPresent = present && !frames[frameIndex].swapchainsToPresent.empty();

        executeChainedCommands(canPresent, frames[frameIndex].fence, nullptr);
        frames[frameIndex].fenceSignaled = true;

        if (canPresent)
            frames[frameIndex].swapchainsToPresent.clear();
    }

    RenderingDevice::RenderingDevice(
        RenderingContextDriver* renderingContext,
        Window* mainWindow
    ) : renderingContextDriver(renderingContext),
        frameIndex(0) {
        Surface* mainSurface = renderingContextDriver->getSurfaceFromWindow(mainWindow);

        const auto devices = renderingContextDriver->getDevices();

        std::string deviceList;
        for (uint32_t i = 0; i < devices.size(); ++i) {
            if (!deviceList.empty())
                deviceList += '\n';

            deviceList += std::format(
                "    [{}] - {}\n"
                "            * Supports presentation? {}",
                i,
                devices[i].name,
                renderingContext->deviceSupportsPresent(i, mainSurface) ? "Yes" : "No"
            );
        }
        spdlog::trace("Found the following devices.\n{}", deviceList);

        uint32_t deviceIndex = std::numeric_limits<uint32_t>::max();
        uint64_t bestDeviceScore = 0;
        for (uint32_t i = 0; i < devices.size(); i++) {
            const auto& deviceOption = devices[i];
            const bool supportsPresent = mainSurface != nullptr
                                             ? renderingContext->deviceSupportsPresent(i, mainSurface)
                                             : false;

            if (!supportsPresent)
                continue;

            uint64_t score = 1;
            switch (deviceOption.type) {
                case DriverDeviceType::Discrete:
                    score += 1'000'000;
                    break;
                case DriverDeviceType::Integrated:
                    score += 500'000;
                    break;
                case DriverDeviceType::Virtual:
                    score += 250'000;
                    break;
                case DriverDeviceType::Cpu:
                    score += 100'000;
                    break;
                case DriverDeviceType::Other:
                    break;
            }

            constexpr uint64_t mebibyte = 1024 * 1024;
            score += std::min(deviceOption.deviceLocalMemory / mebibyte, 100'000ull);
            score += deviceOption.hasDedicatedComputeQueue ? 25'000 : 0;
            score += deviceOption.hasDedicatedTransferQueue ? 25'000 : 0;

            if (deviceIndex == std::numeric_limits<uint32_t>::max() || score > bestDeviceScore) {
                deviceIndex = i;
                bestDeviceScore = score;
            }
        }

        if (deviceIndex == std::numeric_limits<uint32_t>::max())
            error<CantCreateError>("No suitable device found.");

        uint32_t frameCount = 2;

        device = devices[deviceIndex];
        renderingDeviceDriver = renderingContext->createRenderingDeviceDriver(deviceIndex, frameCount);

        graphicsQueueFamily = renderingDeviceDriver->getQueueFamily(
            QueueFamilyBits::Graphics | QueueFamilyBits::Compute,
            nullptr
        ).value();
        graphicsQueue = renderingDeviceDriver->createCommandQueue(graphicsQueueFamily).value();

        transferQueueFamily = renderingDeviceDriver->getQueueFamily(QueueFamilyBits::Transfer, nullptr).value();
        transferQueue = renderingDeviceDriver->createCommandQueue(transferQueueFamily).value();

        frames.reserve(frameCount);
        for (uint32_t i = 0; i < frameCount; i++) {
            const auto commandPool = renderingDeviceDriver->createCommandPool(
                graphicsQueueFamily,
                CommandBufferType::Primary
            );
            if (!commandPool)
                throw CantCreateError("Failed to allocate command pool for frame");

            frames.push_back(
                {
                    .commandPool = commandPool.value(),
                    .commandBuffer = renderingDeviceDriver->createCommandBuffer(commandPool.value()).value(),
                    .fence = renderingDeviceDriver->createFence().value(),
                    .fenceSignaled = false,
                    .waitSemaphores = {},
                    .swapchainsToPresent = {}
                }
            );
        }
        framesDrawn = frames.size();

        renderingDeviceDriver->beginCommandBuffer(frames[0].commandBuffer);
    }

    RenderingDevice::~RenderingDevice() {
        if (!frames.empty())
            flushAndWaitForFrames();

        for (const auto& frame : frames) {
            renderingDeviceDriver->destroyCommandPool(frame.commandPool);
            renderingDeviceDriver->destroyFence(frame.fence);
            delete frame.commandBuffer;
        }
        frames.clear();

        if (transferQueue)
            if (graphicsQueue != transferQueue)
                renderingDeviceDriver->destroyCommandQueue(transferQueue);

        if (graphicsQueue)
            renderingDeviceDriver->destroyCommandQueue(graphicsQueue);

        renderingContextDriver->destroyRenderingDeviceDriver(renderingDeviceDriver);
    }

    void RenderingDevice::swapBuffers(
        const bool present
    ) {
        endFrame();
        executeFrame(present);

        frameIndex = (frameIndex + 1) % frames.size();

        beginFrame(present);
    }

    void RenderingDevice::submit() {
        endFrame();
        executeFrame(false);
    }

    void RenderingDevice::sync() {
        beginFrame(true);
    }

    auto RenderingDevice::createScreen(
        Window* window
    ) -> std::expected<Swapchain*, Error> {
        const auto& surface = renderingContextDriver->getSurfaceFromWindow(window);
        if (surface == nullptr)
            return std::unexpected(Error::InitializationFailed);

        if (swapchains.contains(window))
            return std::unexpected(Error::InitializationFailed);

        const auto& swapchain = renderingDeviceDriver->createSwapchain(surface);
        if (!swapchain)
            return std::unexpected(Error::InitializationFailed);

        swapchains[window] = swapchain.value();

        return swapchain;
    }

    auto RenderingDevice::prepareScreenForDrawing(
        Window* window
    ) -> std::expected<Framebuffer*, Error> {
        const auto& pair = swapchains.find(window);
        DEBUG_ASSERT(pair != swapchains.end());
        const auto& swapchain = pair->second;

        uint32_t toPresentIndex = 0;
        while (toPresentIndex < frames[frameIndex].swapchainsToPresent.size()) {
            if (frames[frameIndex].swapchainsToPresent[toPresentIndex] == swapchain) {
                if (!renderingDeviceDriver->executeCommandQueueAndPresent(graphicsQueue, {}, {}, {}, {}, {swapchain}))
                    return std::unexpected(Error::InitializationFailed);

                frames[frameIndex].swapchainsToPresent.erase(frames[frameIndex].swapchainsToPresent.begin() + toPresentIndex);
            } else {
                toPresentIndex++;
            }
        }

        auto framebuffer = renderingDeviceDriver->acquireSwapchainFramebuffer(graphicsQueue, swapchain);
        if (!framebuffer && framebuffer.error() == SwapchainError::ResizeRequired) {
            flushAndWaitForFrames();

            if (!renderingDeviceDriver->resizeSwapchain(graphicsQueue, swapchain, frames.size()))
                return std::unexpected(Error::InitializationFailed);

            framebuffer = renderingDeviceDriver->acquireSwapchainFramebuffer(graphicsQueue, swapchain);
        }

        if (!framebuffer)
            return std::unexpected(Error::InitializationFailed);

        frames[frameIndex].swapchainsToPresent.push_back(swapchain);

        return framebuffer.value();
    }

    void RenderingDevice::destroyScreen(
        Window* window
    ) {
        const auto& pair = swapchains.find(window);
        if (pair == swapchains.end())
            throw std::invalid_argument("Window does not have an associated swapchain");

        flushAndWaitForFrames();

        renderingDeviceDriver->destroySwapchain(pair->second);
        swapchains.erase(window);
    }

    RenderingContextDriver* RenderingDevice::getRenderingContextDriver() const {
        return renderingContextDriver;
    }

    RenderingDeviceDriver* RenderingDevice::getRenderingDeviceDriver() const {
        return renderingDeviceDriver;
    }
}
