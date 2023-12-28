#pragma once

namespace Vixen {
    namespace Vk {
        class VkImage;
    }

    enum class CommandBufferLevel {
        PRIMARY,
        SECONDARY
    };

    enum class CommandBufferUsage {
        SINGLE,
        SIMULTANEOUS,
        RENDER_PASS_CONTINUE
    };

    template <class Buffer, class Image> requires std::is_base_of_v<Vixen::Buffer, Buffer>
    class CommandBuffer {
    public:
        virtual CommandBuffer& wait() = 0;

        virtual CommandBuffer& reset() = 0;

        virtual CommandBuffer& begin(CommandBufferUsage usage) = 0;

        virtual CommandBuffer& end() = 0;

        virtual void submit() = 0;

        virtual CommandBuffer& copyBuffer(const Buffer& source, const Buffer& destination) = 0;

        virtual CommandBuffer& copyBufferToImage(const Buffer& source, const Image& destination) = 0;

        virtual CommandBuffer& copyImage(const Image& source, const Image& destination) = 0;
    };
}
