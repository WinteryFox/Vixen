#include "RenderingDevice.h"

#include <algorithm>
#include <ranges>
#include <spdlog/spdlog.h>

#include "error/Macros.h"

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
        const bool separatePresentQueue = graphicsQueue != presentQueue;

        Semaphore* semaphore = canPresent && separatePresentQueue ? frames[frameIndex].semaphore : nullptr;
        const bool presentSwapchain = canPresent && !separatePresentQueue;

        executeChainedCommands(presentSwapchain, frames[frameIndex].fence, semaphore);
        frames[frameIndex].fenceSignaled = true;

        if (canPresent) {
            if (separatePresentQueue) {
                if (!renderingDeviceDriver->executeCommandQueueAndPresent(
                    presentQueue,
                    {},
                    {},
                    {},
                    nullptr,
                    frames[frameIndex].swapchainsToPresent
                ))
                    throw std::runtime_error("Command execution failed");
            }

            frames[frameIndex].swapchainsToPresent.clear();
        }
    }

    RenderingDevice::RenderingDevice(
        RenderingContextDriver* renderingContext,
        Window* mainWindow
    ) : renderingContextDriver(renderingContext),
        frameIndex(0) {
        Surface* mainSurface = renderingContextDriver->getSurfaceFromWindow(mainWindow);

        const auto devices = renderingContextDriver->getDevices();

        spdlog::trace(
            "Found the following devices.\n{}",
            std::ranges::fold_left(
                std::views::iota(0) |
                std::views::take(devices.size()) |
                std::views::transform(
                    [&](
                    const std::size_t i
                ) {
                        const auto& [deviceName] = devices[i];
                        return std::format(
                            "    [{}] - {}\n"
                            "            * Supports presentation? {}",
                            std::to_string(i),
                            deviceName,
                            renderingContext->deviceSupportsPresent(i, mainSurface) ? "Yes" : "No"
                        );
                    }
                ),
                std::string{},
                [](
                const auto& a,
                const auto& b
            ) {
                    return a.empty() ? std::move(b) : std::move(a) + "\n" + b;
                }
            )
        );

        uint32_t deviceIndex = 0;
        uint32_t deviceScore = 0;
        for (uint32_t i = 0; i < devices.size(); i++) {
            const auto& deviceOption = devices[i];
            const bool supportsPresent = mainSurface != nullptr
                                             ? renderingContext->deviceSupportsPresent(i, mainSurface)
                                             : false;

            // TODO: Score devices
        }

        uint32_t frameCount = 2;

        device = devices[deviceIndex];
        renderingDeviceDriver = renderingContext->createRenderingDeviceDriver(deviceIndex, frameCount);

        graphicsQueueFamily = renderingDeviceDriver->getQueueFamily(
            QueueFamilyFlags::Graphics | QueueFamilyFlags::Compute,
            nullptr
        ).value();
        graphicsQueue = renderingDeviceDriver->createCommandQueue(graphicsQueueFamily).value();

        transferQueueFamily = renderingDeviceDriver->getQueueFamily(QueueFamilyFlags::Transfer, nullptr).value();
        transferQueue = renderingDeviceDriver->createCommandQueue(transferQueueFamily).value();

        presentQueueFamily = renderingDeviceDriver->getQueueFamily(static_cast<QueueFamilyFlags>(0), mainSurface)
                                                  .value();
        presentQueue = renderingDeviceDriver->createCommandQueue(presentQueueFamily).value();

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
                    .semaphore = renderingDeviceDriver->createSemaphore().value(),
                    .fence = renderingDeviceDriver->createFence().value(),
                    .fenceSignaled = false,
                    .waitSemaphores = {},
                    .swapchainsToPresent = {}
                }
            );
        }
        framesDrawn = frames.size();
    }

    RenderingDevice::~RenderingDevice() {
        if (!frames.empty())
            flushAndWaitForFrames();

        for (const auto& frame : frames) {
            renderingDeviceDriver->destroyCommandPool(frame.commandPool);
            renderingDeviceDriver->destroySemaphore(frame.semaphore);
            renderingDeviceDriver->destroyFence(frame.fence);
            delete frame.commandBuffer;
        }
        frames.clear();

        if (presentQueue)
            if (graphicsQueue != presentQueue)
                renderingDeviceDriver->destroyCommandQueue(presentQueue);

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

        const auto& swapchain = renderingDeviceDriver->createSwapchain(surface);
        if (!swapchain)
            return std::unexpected(Error::InitializationFailed);

        if (!renderingDeviceDriver->resizeSwapchain(graphicsQueue, swapchain.value(), frames.size()))
            return std::unexpected(Error::InitializationFailed);

        swapchains[window] = swapchain.value();

        return swapchain;
    }

    auto RenderingDevice::prepareScreenForDrawing(
        Window* window
    ) -> std::expected<void, Error> {
        const auto& pair = swapchains.find(window);
        DEBUG_ASSERT(pair != swapchains.end());
        const auto& swapchain = pair->second;

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

        return {};
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
