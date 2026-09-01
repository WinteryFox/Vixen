#include "RenderingDevice.h"

#include <algorithm>
#include <bit>
#include <format>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <spdlog/spdlog.h>

#include "RenderingContextDriver.h"
#include "RenderingDeviceDriver.h"
#include "core/error/CantCreateError.h"
#include "core/error/Macros.h"
#include "core/error/SwapchainError.h"

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

    void RenderingDevice::drainDeferredReleases(Frame& frame) {
        auto releases = std::move(frame.deferredReleases);

        frame.deferredReleases.clear();

        for (auto& release : releases)
            release(*renderingDeviceDriver);
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
        drainDeferredReleases(frames[frameIndex]);

        if (!renderingDeviceDriver->resetCommandPool(frames[frameIndex].commandPool))
            throw std::runtime_error("Failed to reset command pool");
        if (!renderingDeviceDriver->beginCommandBuffer(frames[frameIndex].commandBuffer))
            throw std::runtime_error("Failed to begin command buffer");
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
                    .deferredReleases = {},
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

        for (auto& frame : frames)
            drainDeferredReleases(frame);

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

    void RenderingDevice::deferRelease(DeferredRelease release) {
        if (release)
            frames[frameIndex].deferredReleases.push_back(std::move(release));
    }

    void RenderingDevice::deferDestroy(Image* image) {
        if (image == nullptr)
            return;

        deferRelease([image](RenderingDeviceDriver& driver) {
            driver.destroyImage(image);
        });
    }

    void RenderingDevice::deferDestroy(Buffer* buffer) {
        if (buffer == nullptr)
            return;

        deferRelease([buffer](RenderingDeviceDriver& driver) {
            driver.destroyBuffer(buffer);
        });
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

                frames[frameIndex].swapchainsToPresent.erase(
                    frames[frameIndex].swapchainsToPresent.begin() + toPresentIndex);
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

    auto RenderingDevice::createBuffer(
        const uint64_t size,
        const BufferUsageFlags usage,
        const MemoryAllocationType memoryType
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        if (size == 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Cannot create a buffer with a size of 0"
                }
            };

        if (usage.empty())
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "Cannot create a buffer with an empty usage mask"
                }
            };

        constexpr auto knownBufferUsages = BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            BufferUsageBits::UniformTexel |
            BufferUsageBits::StorageTexel |
            BufferUsageBits::Uniform |
            BufferUsageBits::Storage |
            BufferUsageBits::Vertex |
            BufferUsageBits::Index |
            BufferUsageBits::Indirect;

        if (const auto unknownUsageBits = usage.value() & ~knownBufferUsages.value();
            unknownUsageBits != 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = std::format(
                        "Buffer usage contains unknown bits ({:#x})",
                        unknownUsageBits
                    )
                }
            };

        switch (memoryType) {
            case MemoryAllocationType::Cpu:
            case MemoryAllocationType::Gpu:
                break;

            default:
                return std::unexpected{
                    ResourceCreationError{
                        .code = ResourceCreationErrorCode::InvalidDescription,
                        .message = "Buffer memory allocation type is not recognized"
                    }
                };
        }

        const auto maxBufferSize = renderingDeviceDriver->getMaxBufferSize();
        if (size > maxBufferSize)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                    .message = std::format(
                        "Buffer size of {} bytes exceeds the device limit of {} bytes",
                        size,
                        maxBufferSize
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = "maxBufferSize",
                        .requested = size,
                        .supported = maxBufferSize
                    }
                }
            };

        const auto buffer = renderingDeviceDriver->createBuffer(
            size,
            usage,
            memoryType
        );
        if (!buffer)
            return std::unexpected{std::move(buffer).error()};

        if (*buffer == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::NativeObjectCreationFailed,
                    .message = "The rendering backend reported successful buffer creation but returned a null buffer"
                }
            };

        return *buffer;
    }

    auto RenderingDevice::createVertexBuffer(
        const uint64_t size
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createBuffer(
            size,
            BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            BufferUsageBits::Vertex,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createUniformBuffer(
        const uint64_t size
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createBuffer(
            size,
            BufferUsageBits::CopyDestination |
            BufferUsageBits::Uniform,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createStorageBuffer(
        const uint64_t size
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createBuffer(
            size,
            BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            BufferUsageBits::Storage,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createTexelBuffer(
        const uint32_t elementCount,
        const ImageDataFormat format,
        const BufferUsageBits usage
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        if (elementCount == 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "A texel buffer must contain at least one texel"
                }
            };

        if (usage != BufferUsageBits::UniformTexel && usage != BufferUsageBits::StorageTexel)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = "A texel buffer must use either UniformTexel or StorageTexel usage"
                }
            };

        const auto texelSize = getTexelSize(format);
        if (texelSize == 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedFormat,
                    .message = "The requested format cannot be used as a texel-buffer format"
                }
            };

        const auto supportedUsages = renderingDeviceDriver->getTexelBufferUsageSupportedByFormat(format);
        if (!supportedUsages)
            return std::unexpected{std::move(supportedUsages).error()};

        if (!supportedUsages->contains(usage))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = usage == BufferUsageBits::UniformTexel
                                   ? "Uniform texel-buffer access is not supported for the requested format"
                                   : "Storage texel-buffer access is not supported for the requested format"
                }
            };

        const auto maxElements = renderingDeviceDriver->getMaxTexelBufferElements();
        if (elementCount > maxElements)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::ExceedsDeviceLimits,
                    .message = std::format(
                        "Texel-buffer element count {} exceeds the device limit of {}",
                        elementCount,
                        maxElements
                    ),
                    .limitViolation = ResourceCreationLimitViolation{
                        .limit = "maxTexelBufferElements",
                        .requested = elementCount,
                        .supported = maxElements
                    }
                }
            };

        return createBuffer(
            static_cast<uint64_t>(elementCount) * texelSize,
            BufferUsageBits::CopySource |
            BufferUsageBits::CopyDestination |
            usage,
            MemoryAllocationType::Gpu
        );
    }

    auto RenderingDevice::createUniformTexelBuffer(
        const uint32_t elementCount,
        const ImageDataFormat format
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createTexelBuffer(elementCount, format, BufferUsageBits::UniformTexel);
    }

    auto RenderingDevice::createStorageTexelBuffer(
        const uint32_t elementCount,
        const ImageDataFormat format
    ) const -> std::expected<Buffer*, ResourceCreationError> {
        return createTexelBuffer(elementCount, format, BufferUsageBits::StorageTexel);
    }

    auto RenderingDevice::createImage(
        const ImageFormat& format,
        const ImageView& view
    ) const -> std::expected<Image*, ResourceCreationError> {
        const auto invalidDescription = [](std::string message)
            -> std::expected<Image*, ResourceCreationError> {
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::InvalidDescription,
                    .message = std::move(message)
                }
            };
        };

        if (format.width == 0 ||
            format.height == 0 ||
            format.depth == 0 ||
            format.mipmapCount == 0 ||
            format.layerCount == 0)
            return invalidDescription(
                "Image width, height, depth, mipmap count, and layer count must all be greater than zero"
            );

        const auto maxMipmapCount = static_cast<uint32_t>(std::bit_width(
            std::max({format.width, format.height, format.depth})
        ));
        if (format.mipmapCount > maxMipmapCount)
            return invalidDescription(
                std::format(
                    "Image requests {} mip levels, but extent {}x{}x{} supports at most {}",
                    format.mipmapCount,
                    format.width,
                    format.height,
                    format.depth,
                    maxMipmapCount
                )
            );

        if (format.usage.empty())
            return invalidDescription("Image usage flags must not be empty");

        if (format.usage.value() == ImageUsageFlags{ImageUsageBits::CpuRead}.value())
            return invalidDescription(
                "CpuRead is a memory-placement capability and must be combined with a device image usage, "
                "such as CopyDestination for a readback image"
            );

        constexpr auto knownImageUsages = ImageUsageBits::Sampling |
            ImageUsageBits::ColorAttachment |
            ImageUsageBits::DepthStencilAttachment |
            ImageUsageBits::Storage |
            ImageUsageBits::AtomicStorage |
            ImageUsageBits::CpuRead |
            ImageUsageBits::Update |
            ImageUsageBits::CopySource |
            ImageUsageBits::CopyDestination |
            ImageUsageBits::InputAttachment |
            ImageUsageBits::TransientAttachment;

        if (const auto unknownUsageBits = format.usage.value() & ~knownImageUsages.value();
            unknownUsageBits != 0)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = std::format(
                        "Image usage contains unknown bits ({:#x})",
                        unknownUsageBits
                    )
                }
            };

        switch (format.type) {
            case ImageType::OneD:
                if (format.height != 1 || format.depth != 1)
                    return invalidDescription(
                        "One-dimensional images must have height 1 and depth 1"
                    );
                if (format.layerCount != 1)
                    return invalidDescription("Non-array one-dimensional images must have exactly one layer");
                break;

            case ImageType::OneDArray:
                if (format.height != 1 || format.depth != 1)
                    return invalidDescription(
                        "One-dimensional image arrays must have height 1 and depth 1"
                    );
                break;

            case ImageType::TwoD:
                if (format.depth != 1)
                    return invalidDescription("Two-dimensional images must have depth 1");
                if (format.layerCount != 1)
                    return invalidDescription("Non-array two-dimensional images must have exactly one layer");
                break;

            case ImageType::TwoDArray:
                if (format.depth != 1)
                    return invalidDescription("Two-dimensional image arrays must have depth 1");
                break;

            case ImageType::ThreeD:
                if (format.layerCount != 1)
                    return invalidDescription("Three-dimensional images must have exactly one array layer");
                break;

            case ImageType::Cube:
                if (format.depth != 1)
                    return invalidDescription("Cube images must have depth 1");
                if (format.width != format.height)
                    return invalidDescription("Cube images must have equal width and height");
                if (format.layerCount != 6)
                    return invalidDescription("Cube images must have exactly six array layers");
                break;

            case ImageType::CubeArray:
                if (format.depth != 1)
                    return invalidDescription("Cube-array images must have depth 1");
                if (format.width != format.height)
                    return invalidDescription("Cube-array images must have equal width and height");
                if (format.layerCount < 6 || format.layerCount % 6 != 0)
                    return invalidDescription(
                        "Cube-array images must have a positive multiple of six array layers"
                    );
                break;
        }

        if (format.samples != ImageSamples::One) {
            if (format.type != ImageType::TwoD && format.type != ImageType::TwoDArray)
                return invalidDescription(
                    "Multisampled images must be two-dimensional or two-dimensional arrays"
                );

            if (format.mipmapCount != 1)
                return invalidDescription("Multisampled images must have exactly one mip level");
        }

        const auto supportedUsages = renderingDeviceDriver->getImageUsageSupportedByFormat(
            format.format,
            format.usage.contains(ImageUsageBits::CpuRead)
        );
        if (!supportedUsages)
            return std::unexpected{std::move(supportedUsages).error()};

        if (format.usage.contains(ImageUsageBits::Sampling) &&
            !supportedUsages->contains(ImageUsageBits::Sampling))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Sampling is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::ColorAttachment) &&
            !supportedUsages->contains(ImageUsageBits::ColorAttachment))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Color attachment is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::DepthStencilAttachment) &&
            !supportedUsages->contains(ImageUsageBits::DepthStencilAttachment))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Depth-stencil attachment is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::Storage) &&
            !supportedUsages->contains(ImageUsageBits::Storage))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Storage is not supported for this format"
                }
            };

        if (format.usage.contains(ImageUsageBits::AtomicStorage) &&
            !supportedUsages->contains(ImageUsageBits::AtomicStorage))
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::UnsupportedUsage,
                    .message = "Atomic storage-image access is not supported for this format"
                }
            };

        const auto image = renderingDeviceDriver->createImage(format, view);
        if (!image)
            return std::unexpected{std::move(image).error()};

        if (*image == nullptr)
            return std::unexpected{
                ResourceCreationError{
                    .code = ResourceCreationErrorCode::NativeObjectCreationFailed,
                    .message = "The rendering backend reported successful image creation but returned a null image"
                }
            };

        return *image;
    }

    RenderingContextDriver* RenderingDevice::getRenderingContextDriver() const {
        return renderingContextDriver;
    }

    RenderingDeviceDriver* RenderingDevice::getRenderingDeviceDriver() const {
        return renderingDeviceDriver;
    }
}
