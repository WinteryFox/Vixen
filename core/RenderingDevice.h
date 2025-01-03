#pragma once

#include <functional>
#include <string>

#include "command/CommandPool.h"
#include "command/CommandBuffer.h"
#include "buffer/Buffer.h"
#include "buffer/BufferUsage.h"
#include "image/Image.h"
#include "image/ImageFormat.h"
#include "image/ImageView.h"
#include "image/Sampler.h"
#include "image/SamplerState.h"
#include "shader/Shader.h"
#include "shader/ShaderLanguage.h"

namespace Vixen {
    class RenderingDevice {
    public:
        struct ShaderStageData {
            ShaderStage stage;
            std::vector<std::byte> spirv;
        };

    protected:
        static bool reflectShader(const std::vector<ShaderStageData> &stages, Shader *shader);

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

        virtual std::vector<std::byte> compileSpirvFromSource(ShaderStage stage, const std::string &source,
                                                              ShaderLanguage language);

        virtual Shader *createShaderFromSpirv(const std::string &name,
                                              const std::vector<ShaderStageData> &stages) = 0;

        virtual void destroyShaderModules(Shader *shader) = 0;

        virtual void destroyShader(Shader *shader) = 0;
    };
}
