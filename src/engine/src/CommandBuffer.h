#pragma once

namespace Vixen {
    namespace Vk {
        enum class CommandBufferUsage;
        class Buffer;
        class Image;
    }

    class CommandBuffer {
    public:
        virtual ~CommandBuffer() = default;

        virtual CommandBuffer& wait() = 0;

        virtual CommandBuffer& reset() = 0;

        virtual CommandBuffer& begin(Vk::CommandBufferUsage usage) = 0;

        virtual CommandBuffer& end() = 0;

        virtual void submit() = 0;

        virtual CommandBuffer& copyBuffer(const Vk::Buffer& source, const Vk::Buffer& destination) = 0;

        virtual CommandBuffer& copyBufferToImage(const Vk::Buffer& source, const Vk::Image& destination) = 0;

        virtual CommandBuffer& copyImage(const Vk::Image& source, const Vk::Image& destination) = 0;
    };
}
