#include "RenderingDevice.h"

#include <ranges>
#include <bits/ranges_algo.h>
#include <spdlog/spdlog.h>

#include "error/Macros.h"

namespace Vixen {
    void RenderingDevice::waitForFrame(const uint32_t frameIndex) {
        if (!frames[frameIndex].fenceSignaled)
            return;

        renderingDeviceDriver->waitOnFence(frames[frameIndex].fence).value();
        frames[frameIndex].fenceSignaled = false;
    }

    void RenderingDevice::beginFrame(const bool presented) {
        waitForFrame(frameIndex);

        renderingDeviceDriver->resetCommandPool(frames[frameIndex].commandPool);
        renderingDeviceDriver->beginCommandBuffer(frames[frameIndex].commandBuffer);

        // TODO: Free this frame's resources
    }

    void RenderingDevice::endFrame() {
        renderingDeviceDriver->endCommandBuffer(frames[frameIndex].commandBuffer);
    }

    void RenderingDevice::executeChainedCommands(const bool present, Fence *drawFence,
                                                 Semaphore *drawSemaphoreToSignal) {
        renderingDeviceDriver->executeCommandQueueAndPresent(
            graphicsQueue,
            frames[frameIndex].waitSemaphores,
            frames[frameIndex].commandBuffer
                ? std::vector{frames[frameIndex].commandBuffer}
                : std::vector<CommandBuffer *>{},
            drawSemaphoreToSignal
                ? std::vector{drawSemaphoreToSignal}
                : std::vector<Semaphore *>{},
            drawFence,
            present
                ? frames[frameIndex].swapchainsToPresent
                : std::vector<Swapchain *>{}
        );

        frames[frameIndex].waitSemaphores.clear();
    }

    void RenderingDevice::executeFrame(const bool present) {
        const bool canPresent = present && !frames[frameIndex].swapchainsToPresent.empty();
        const bool separatePresentQueue = graphicsQueue != presentQueue;

        Semaphore *semaphore = canPresent && separatePresentQueue ? frames[frameIndex].semaphore : nullptr;
        const bool presentSwapchain = canPresent && !separatePresentQueue;

        executeChainedCommands(presentSwapchain, frames[frameIndex].fence, semaphore);
        frames[frameIndex].fenceSignaled = true;

        if (canPresent) {
            if (separatePresentQueue) {
                renderingDeviceDriver->executeCommandQueueAndPresent(
                    presentQueue,
                    {frames[frameIndex].semaphore},
                    {},
                    {},
                    nullptr,
                    frames[frameIndex].swapchainsToPresent
                );
            }

            frames[frameIndex].swapchainsToPresent.clear();
        }
    }

    RenderingDevice::RenderingDevice(RenderingContextDriver *renderingContext, Window *mainWindow)
        : renderingContextDriver(renderingContext),
          frameIndex(0) {
        Surface *mainSurface = nullptr;
        if (mainWindow) {
            DEBUG_ASSERT(mainWindow->surface != nullptr);
            mainSurface = mainWindow->surface;
        }

        const auto devices = renderingContextDriver->getDevices();

        spdlog::trace(
            "Found the following devices.\n{}",
            std::ranges::fold_left(
                std::views::iota(0) |
                std::views::take(devices.size()) |
                std::views::transform([&](const std::size_t i) {
                    const auto &[deviceName] = devices[i];
                    return std::format(
                        "    [{}] - {}\n"
                        "            * Supports presentation? {}",
                        std::to_string(i),
                        deviceName,
                        renderingContext->deviceSupportsPresent(i, mainSurface) ? "Yes" : "No"
                    );
                }),
                std::string{},
                [](const auto &a, const auto &b) {
                    return a.empty() ? std::move(b) : std::move(a) + "\n" + b;
                }
            )
        );

        uint32_t deviceIndex = 0;
        uint32_t deviceScore = 0;
        for (uint32_t i = 0; i < devices.size(); i++) {
            const auto &deviceOption = devices[i];
            const bool supportsPresent = mainSurface != nullptr
                                             ? renderingContext->deviceSupportsPresent(i, mainSurface)
                                             : false;

            // TODO: Score devices
        }

        uint32_t frameCount = 1;

        device = devices[deviceIndex];
        renderingDeviceDriver = renderingContext->createRenderingDeviceDriver(deviceIndex, frameCount);

        graphicsQueueFamily = renderingDeviceDriver->getQueueFamily(
            QueueFamilyFlags::Graphics | QueueFamilyFlags::Compute, nullptr).value();
        graphicsQueue = renderingDeviceDriver->createCommandQueue(graphicsQueueFamily).value();

        transferQueueFamily = renderingDeviceDriver->getQueueFamily(QueueFamilyFlags::Transfer, nullptr).value();
        transferQueue = renderingDeviceDriver->createCommandQueue(transferQueueFamily).value();

        presentQueueFamily = renderingDeviceDriver->getQueueFamily(static_cast<QueueFamilyFlags>(0), mainSurface)
                .value();
        presentQueue = renderingDeviceDriver->createCommandQueue(presentQueueFamily).value();

        frames.reserve(frameCount);
        for (uint32_t i = 0; i < frameCount; i++) {
            const auto commandPool = renderingDeviceDriver->createCommandPool(
                graphicsQueueFamily, CommandBufferType::Primary);

            frames.push_back({
                .commandPool = commandPool,
                .commandBuffer = renderingDeviceDriver->createCommandBuffer(commandPool),
                .semaphore = renderingDeviceDriver->createSemaphore(),
                .fence = renderingDeviceDriver->createFence(),
                .fenceSignaled = false,
                .waitSemaphores = {},
                .swapchainsToPresent = {},
                .transferSemaphores = {}
            });
        }
        framesDrawn = frames.size();
    }

    RenderingDevice::~RenderingDevice() {
        for (const auto &frame: frames) {
            renderingDeviceDriver->destroySemaphore(frame.semaphore);
            renderingDeviceDriver->destroyFence(frame.fence);
            renderingDeviceDriver->destroyCommandPool(frame.commandPool);
        }

        renderingDeviceDriver->destroyCommandQueue(graphicsQueue);
        renderingDeviceDriver->destroyCommandQueue(transferQueue);
        renderingDeviceDriver->destroyCommandQueue(presentQueue);
        delete renderingDeviceDriver;
        delete renderingContextDriver;
    }

    void RenderingDevice::swapBuffers(const bool present) {
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

    RenderingContextDriver *RenderingDevice::getRenderingContextDriver() const {
        return renderingContextDriver;
    }

    RenderingDeviceDriver *RenderingDevice::getRenderingDeviceDriver() const {
        return renderingDeviceDriver;
    }
}
