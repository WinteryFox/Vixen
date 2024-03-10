#pragma once

namespace Vixen {
    namespace Vk {
        class VkBuffer;
        class VkImage;
    }

    enum class CommandBufferLevel {
        Primary,
        Secondary
    };

    enum class CommandBufferUsage {
        Once,
        Simultanious
    };

    class CommandBuffer {
    public:
        virtual ~CommandBuffer() = default;

        virtual CommandBuffer& wait() = 0;

        virtual CommandBuffer& reset() = 0;

        virtual CommandBuffer& begin(CommandBufferUsage usage) = 0;

        virtual CommandBuffer& end() = 0;

        virtual void submit() = 0;

        virtual CommandBuffer& copyBuffer(const Vk::VkBuffer& source, const Vk::VkBuffer& destination) = 0;

        virtual CommandBuffer& copyBufferToImage(const Vk::VkBuffer& source, const Vk::VkImage& destination) = 0;

        virtual CommandBuffer& copyImage(const Vk::VkImage& source, const Vk::VkImage& destination) = 0;
    };
}
