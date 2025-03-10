#include "RenderingDevice.h"

#include <ranges>
#include <bits/ranges_algo.h>
#include <spdlog/spdlog.h>

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
                                                 Semaphore *drawSemaphoresToSignal) {
        std::vector<Swapchain *> swapchains{};
        std::vector<Semaphore *> waitSemaphores = frames[frameIndex].waitSemaphores;

        if (present)
            swapchains = frames[frameIndex].swapchainsToPresent;

        renderingDeviceDriver->executeCommandQueueAndPresent(
            graphicsQueue,
            waitSemaphores,
            frames[frameIndex].commandBuffer
                ? std::vector{frames[frameIndex].commandBuffer}
                : std::vector<CommandBuffer *>{},
            drawSemaphoresToSignal
                ? std::vector{drawSemaphoresToSignal}
                : std::vector<Semaphore *>{},
            drawFence,
            swapchains
        );

        waitSemaphores.resize(1);
        waitSemaphores[0] = drawSemaphoresToSignal;

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
                    {frames[frameIndex].swapchainsToPresent}
                );
            }

            frames[frameIndex].swapchainsToPresent.clear();
        }
    }

    RenderingDevice::RenderingDevice(RenderingContextDriver *renderingContext, Window *window)
        : window(window),
          renderingContextDriver(renderingContext),
          frameIndex(0) {
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
                        renderingContext->deviceSupportsPresent(i, window->surface) ? "Yes" : "No"
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
            const bool supportsPresent = window->surface != nullptr
                                             ? renderingContext->deviceSupportsPresent(i, window->surface)
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

        presentQueueFamily = renderingDeviceDriver->getQueueFamily(static_cast<QueueFamilyFlags>(0), window->surface)
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
                .fenceSignaled = false
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
