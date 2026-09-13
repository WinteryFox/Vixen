#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "pipeline/PipelineLayout.h"

namespace Vixen {
    class DescriptorPool;
    class DescriptorSet;
    class RenderingDeviceDriver;
    class VulkanRenderingDeviceDriver;

    class DescriptorPoolLifetime {
        friend class DescriptorPool;
        friend class DescriptorSet;
        friend class RenderingDeviceDriver;
        friend class VulkanRenderingDeviceDriver;

        DescriptorPool* owner = nullptr;
        uint64_t generation = 1;
    };

    struct DescriptorElementState {
        bool initialized = false;
    };

    struct DescriptorBindingState {
        uint32_t binding = 0;
        ShaderUniformType type = ShaderUniformType::Sampler;
        std::vector<DescriptorElementState> elements;
    };

    struct DescriptorSetRecord {
        const PipelineLayout* pipelineLayout = nullptr;
        DescriptorSetLayoutDescription layout;
        uint32_t set = 0;
        std::vector<DescriptorBindingState> bindings;
    };

    class DescriptorPool {
        friend class RenderingDeviceDriver;
        friend class VulkanRenderingDeviceDriver;
        friend class DescriptorSet;

        std::shared_ptr<DescriptorPoolLifetime> lifetime;
        std::vector<DescriptorSetRecord> allocations;
        bool poisoned = false;

    protected:
        DescriptorPool();

    public:
        DescriptorPool(DescriptorPool const&) = delete;
        DescriptorPool& operator=(DescriptorPool const&) = delete;

        DescriptorPool(DescriptorPool&&) = delete;
        DescriptorPool& operator=(DescriptorPool&&) = delete;

        virtual ~DescriptorPool();
    };
}
