#pragma once

#include <functional>
#include <string>

#include "command/CommandPool.h"
#include "command/CommandBuffer.h"
#include "buffer/Buffer.h"
#include "buffer/BufferUsage.h"
#include "command/CommandBufferType.h"
#include "image/Image.h"
#include "image/ImageFormat.h"
#include "image/ImageView.h"
#include "image/Sampler.h"
#include "image/SamplerState.h"
#include "shader/Shader.h"

namespace Vixen {
    class RenderingDevice {
    public:
        virtual ~RenderingDevice() = default;

        virtual CommandPool *createCommandPool(uint32_t queueFamily, CommandBufferType type) = 0;

        virtual void resetCommandPool(CommandPool *pool) = 0;

        virtual void destroyCommandPool(CommandPool *pool) = 0;

        virtual CommandBuffer *createCommandBuffer(CommandPool *pool) = 0;

        virtual Buffer *createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) = 0;

        virtual void destroyBuffer(Buffer *buffer) = 0;

        virtual Image *createImage(const ImageFormat &format, const ImageView &view) = 0;

        virtual std::byte *mapImage(Image *image) = 0;

        virtual void unmapImage(Image *image) = 0;

        virtual void destroyImage(Image *image) = 0;

        virtual Sampler *createSampler(SamplerState state) = 0;

        virtual void destroySampler(Sampler *sampler) = 0;

        static std::vector<std::byte> compileSpirvFromSource(const std::string& name, ShaderStage stage, const std::string &source);

        virtual Shader *createShaderFromBytecode(const std::vector<std::byte> &binary) = 0;

        virtual void destroyShader(Shader *shader) = 0;
    };
}
