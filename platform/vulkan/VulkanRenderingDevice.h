#pragma once

#include <memory>
#include <vector>
#include <volk.h>

#include "core/RenderingDevice.h"
#include "device/GraphicsCard.h"

typedef struct VmaAllocator_T *VmaAllocator;

namespace Vixen {
    class VulkanRenderingContext;

    class VulkanRenderingDevice final : public RenderingDevice {
        struct Features {
            bool dynamicRendering;
            bool deviceFault;
        } enabledFeatures;

        std::shared_ptr<VulkanRenderingContext> renderingContext;

        GraphicsCard physicalDevice;

        std::vector<const char *> enabledExtensionNames;

        VkDevice device;

        VmaAllocator allocator;

        void initializeExtensions();

        void checkFeatures() const;

        void checkCapabilities();

        void initializeDevice();

    public:
        VulkanRenderingDevice(const std::shared_ptr<VulkanRenderingContext> &renderingContext, uint32_t deviceIndex);

        ~VulkanRenderingDevice() override;

        Buffer createBuffer(Buffer::Usage usage, uint32_t count, uint32_t stride) override;

        Image createImage(const ImageFormat &format, const ImageView &view) override;

        [[nodiscard]] GraphicsCard getPhysicalDevice() const;

        [[nodiscard]] VkDevice getDevice() const;

        [[nodiscard]] VmaAllocator getAllocator() const;
    };
}
