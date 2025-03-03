#include "RenderingDevice.h"

#include <spdlog/spdlog.h>

namespace Vixen {
    void RenderingDevice::beginFrame() {
    }

    void RenderingDevice::endFrame() {
    }

    void RenderingDevice::executeFrame(bool present) {
        renderingDeviceDriver->executeCommandQueueAndPresent(
            graphicsQueue,
            {},
            {},
            {},
            nullptr,
            {}
        );
    }

    RenderingDevice::RenderingDevice(RenderingContextDriver *renderingContext, Window *window)
        : renderingContextDriver(renderingContext) {
        spdlog::trace("Found devices:");
        uint32_t deviceIndex = 0;
        uint32_t deviceScore = 0;
        const auto devices = renderingContextDriver->getDevices();
        for (uint32_t i = 0; i < devices.size(); i++) {
            auto deviceOption = devices[i];
            bool supportsPresent = window->surface != nullptr
                                       ? renderingContext->deviceSupportsPresent(i, window->surface)
                                       : false;
            spdlog::trace("    {} - Supports present? {}", deviceOption.name, supportsPresent);

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

        presentQueueFamily = renderingDeviceDriver->getQueueFamily(static_cast<QueueFamilyFlags>(0), window->surface).
                value();
        presentQueue = renderingDeviceDriver->createCommandQueue(presentQueueFamily).value();

        const auto commandPool = renderingDeviceDriver->createCommandPool(
            graphicsQueueFamily, CommandBufferType::Primary);

        frame = 0;
        frames.reserve(frameCount);
        for (uint32_t i = 0; i < frameCount; i++) {
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

        frame = (frame + 1) % frames.size();

        beginFrame();
    }

    void RenderingDevice::submit() {
        endFrame();
        executeFrame(false);
    }

    void RenderingDevice::sync() {
        beginFrame();
    }

    RenderingContextDriver *RenderingDevice::getRenderingContextDriver() const {
        return renderingContextDriver;
    }

    RenderingDeviceDriver *RenderingDevice::getRenderingDeviceDriver() const {
        return renderingDeviceDriver;
    }
}
