#pragma once

#include <memory>
#include <vector>

#include "core/RenderingDevice.h"
#include "GraphicsCard.h"

typedef struct VmaAllocator_T *VmaAllocator;

namespace Vixen {
    enum class BufferUsage : int64_t;
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

        [[nodiscard]] VkSampleCountFlagBits findClosestSupportedSampleCount(const ImageSamples &samples) const;

    public:
        VulkanRenderingDevice(const std::shared_ptr<VulkanRenderingContext> &renderingContext, uint32_t deviceIndex);

        VulkanRenderingDevice(const VulkanRenderingDevice &) = delete;

        VulkanRenderingDevice &operator=(const VulkanRenderingDevice &) = delete;

        VulkanRenderingDevice(VulkanRenderingDevice &&) = delete;

        VulkanRenderingDevice &operator=(VulkanRenderingDevice &&) = delete;

        ~VulkanRenderingDevice() override;

        CommandPool *createCommandPool(uint32_t queueFamily, CommandBufferType type) override;

        void resetCommandPool(CommandPool *pool) override;

        void destroyCommandPool(CommandPool *pool) override;

        CommandBuffer *createCommandBuffer(CommandPool *pool) override;

        Buffer *createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) override;

        void destroyBuffer(Buffer *buffer) override;

        Image *createImage(const ImageFormat &format, const ImageView &view) override;

        std::byte *mapImage(Image *image) override;

        void unmapImage(Image *image) override;

        void destroyImage(Image *image) override;

        Sampler *createSampler(SamplerState state) override;

        void destroySampler(Sampler *sampler) override;

        Shader *createShaderFromSpirv(const std::string &name, const std::vector<ShaderStageData> &stages) override;

        void destroyShaderModules(Shader *shader) override;

        void destroyShader(Shader *shader) override;

        [[nodiscard]] GraphicsCard getPhysicalDevice() const;
    };
}
