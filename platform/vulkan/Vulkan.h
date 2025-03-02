#pragma once

#include <vulkan/vk_enum_string_helper.h>

#ifdef DEBUG_ENABLED

#include <spdlog/sinks/stdout_color_sinks.h>

#endif

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "core/BarrierAccessFlags.h"
#include "core/Bitmask.h"
#include "core/LoadAction.h"
#include "core/PipelineStageFlags.h"
#include "core/StoreAction.h"
#include "core/image/ImageAspectFlags.h"
#include "core/image/ImageLayout.h"
#include "core/image/ImageSamples.h"
#include "core/image/ImageSwizzle.h"
#include "core/image/ImageType.h"
#include "core/image/SamplerBorderColor.h"
#include "core/image/SamplerRepeatMode.h"
#include "core/shader/ShaderStage.h"

namespace Vixen {
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Identity, VK_COMPONENT_SWIZZLE_IDENTITY));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Zero, VK_COMPONENT_SWIZZLE_ZERO));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::One, VK_COMPONENT_SWIZZLE_ONE));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Red, VK_COMPONENT_SWIZZLE_R));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Green, VK_COMPONENT_SWIZZLE_G));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Blue, VK_COMPONENT_SWIZZLE_B));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Alpha, VK_COMPONENT_SWIZZLE_A));

    static_assert(
        ENUM_MEMBERS_EQUAL(SamplerBorderColor::FloatTransparentBlack, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::IntTransparentBlack, VK_BORDER_COLOR_INT_TRANSPARENT_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::FloatOpaqueBlack, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::IntOpaqueBlack, VK_BORDER_COLOR_INT_OPAQUE_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::FloatOpaqueWhite, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::IntOpaqueWhite, VK_BORDER_COLOR_INT_OPAQUE_WHITE));

    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::Repeat, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::MirroredRepeat, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::ClampToEdge, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::ClampToBorder, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER));
    static_assert(
        ENUM_MEMBERS_EQUAL(SamplerRepeatMode::MirrorClampToEdge, VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE));

    [[maybe_unused]] static bool isDepthFormat(const VkFormat format) {
        switch (format) {
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return true;

            default:
                return false;
        }
    }

    static constexpr VkFormat toVkDataFormat[] = {
        VK_FORMAT_R4G4_UNORM_PACK8,
        VK_FORMAT_R4G4B4A4_UNORM_PACK16,
        VK_FORMAT_B4G4R4A4_UNORM_PACK16,
        VK_FORMAT_R5G6B5_UNORM_PACK16,
        VK_FORMAT_B5G6R5_UNORM_PACK16,
        VK_FORMAT_R5G5B5A1_UNORM_PACK16,
        VK_FORMAT_B5G5R5A1_UNORM_PACK16,
        VK_FORMAT_A1R5G5B5_UNORM_PACK16,
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R8_USCALED,
        VK_FORMAT_R8_SSCALED,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R8_SINT,
        VK_FORMAT_R8_SRGB,
        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R8G8_USCALED,
        VK_FORMAT_R8G8_SSCALED,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R8G8_SINT,
        VK_FORMAT_R8G8_SRGB,
        VK_FORMAT_R8G8B8_UNORM,
        VK_FORMAT_R8G8B8_SNORM,
        VK_FORMAT_R8G8B8_USCALED,
        VK_FORMAT_R8G8B8_SSCALED,
        VK_FORMAT_R8G8B8_UINT,
        VK_FORMAT_R8G8B8_SINT,
        VK_FORMAT_R8G8B8_SRGB,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_B8G8R8_SNORM,
        VK_FORMAT_B8G8R8_USCALED,
        VK_FORMAT_B8G8R8_SSCALED,
        VK_FORMAT_B8G8R8_UINT,
        VK_FORMAT_B8G8R8_SINT,
        VK_FORMAT_B8G8R8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SNORM,
        VK_FORMAT_R8G8B8A8_USCALED,
        VK_FORMAT_R8G8B8A8_SSCALED,
        VK_FORMAT_R8G8B8A8_UINT,
        VK_FORMAT_R8G8B8A8_SINT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8A8_SNORM,
        VK_FORMAT_B8G8R8A8_USCALED,
        VK_FORMAT_B8G8R8A8_SSCALED,
        VK_FORMAT_B8G8R8A8_UINT,
        VK_FORMAT_B8G8R8A8_SINT,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_A8B8G8R8_UNORM_PACK32,
        VK_FORMAT_A8B8G8R8_SNORM_PACK32,
        VK_FORMAT_A8B8G8R8_USCALED_PACK32,
        VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
        VK_FORMAT_A8B8G8R8_UINT_PACK32,
        VK_FORMAT_A8B8G8R8_SINT_PACK32,
        VK_FORMAT_A8B8G8R8_SRGB_PACK32,
        VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_FORMAT_A2R10G10B10_SNORM_PACK32,
        VK_FORMAT_A2R10G10B10_USCALED_PACK32,
        VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
        VK_FORMAT_A2R10G10B10_UINT_PACK32,
        VK_FORMAT_A2R10G10B10_SINT_PACK32,
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        VK_FORMAT_A2B10G10R10_SNORM_PACK32,
        VK_FORMAT_A2B10G10R10_USCALED_PACK32,
        VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
        VK_FORMAT_A2B10G10R10_UINT_PACK32,
        VK_FORMAT_A2B10G10R10_SINT_PACK32,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R16_SNORM,
        VK_FORMAT_R16_USCALED,
        VK_FORMAT_R16_SSCALED,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16_SFLOAT,
        VK_FORMAT_R16G16_UNORM,
        VK_FORMAT_R16G16_SNORM,
        VK_FORMAT_R16G16_USCALED,
        VK_FORMAT_R16G16_SSCALED,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16_SFLOAT,
        VK_FORMAT_R16G16B16_UNORM,
        VK_FORMAT_R16G16B16_SNORM,
        VK_FORMAT_R16G16B16_USCALED,
        VK_FORMAT_R16G16B16_SSCALED,
        VK_FORMAT_R16G16B16_UINT,
        VK_FORMAT_R16G16B16_SINT,
        VK_FORMAT_R16G16B16_SFLOAT,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R16G16B16A16_SNORM,
        VK_FORMAT_R16G16B16A16_USCALED,
        VK_FORMAT_R16G16B16A16_SSCALED,
        VK_FORMAT_R16G16B16A16_UINT,
        VK_FORMAT_R16G16B16A16_SINT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_UINT,
        VK_FORMAT_R32G32B32_SINT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_UINT,
        VK_FORMAT_R32G32B32A32_SINT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R64_UINT,
        VK_FORMAT_R64_SINT,
        VK_FORMAT_R64_SFLOAT,
        VK_FORMAT_R64G64_UINT,
        VK_FORMAT_R64G64_SINT,
        VK_FORMAT_R64G64_SFLOAT,
        VK_FORMAT_R64G64B64_UINT,
        VK_FORMAT_R64G64B64_SINT,
        VK_FORMAT_R64G64B64_SFLOAT,
        VK_FORMAT_R64G64B64A64_UINT,
        VK_FORMAT_R64G64B64A64_SINT,
        VK_FORMAT_R64G64B64A64_SFLOAT,
        VK_FORMAT_B10G11R11_UFLOAT_PACK32,
        VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_BC1_RGB_UNORM_BLOCK,
        VK_FORMAT_BC1_RGB_SRGB_BLOCK,
        VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
        VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
        VK_FORMAT_BC2_UNORM_BLOCK,
        VK_FORMAT_BC2_SRGB_BLOCK,
        VK_FORMAT_BC3_UNORM_BLOCK,
        VK_FORMAT_BC3_SRGB_BLOCK,
        VK_FORMAT_BC4_UNORM_BLOCK,
        VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,
        VK_FORMAT_BC5_SNORM_BLOCK,
        VK_FORMAT_BC6H_UFLOAT_BLOCK,
        VK_FORMAT_BC6H_SFLOAT_BLOCK,
        VK_FORMAT_BC7_UNORM_BLOCK,
        VK_FORMAT_BC7_SRGB_BLOCK,
        VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
        VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        VK_FORMAT_EAC_R11_UNORM_BLOCK,
        VK_FORMAT_EAC_R11_SNORM_BLOCK,
        VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
        VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
        VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
        VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
        VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
        VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
        VK_FORMAT_G8B8G8R8_422_UNORM,
        VK_FORMAT_B8G8R8G8_422_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
        VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
        VK_FORMAT_R10X6_UNORM_PACK16,
        VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
        VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
        VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
        VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        VK_FORMAT_R12X4_UNORM_PACK16,
        VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
        VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
        VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
        VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
        VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        VK_FORMAT_G16B16G16R16_422_UNORM,
        VK_FORMAT_B16G16R16G16_422_UNORM,
        VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
        VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,
        VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
        VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,
        VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM
    };

    static VkSampleCountFlagBits toVkSampleCountFlagBits(const ImageSamples &samples) {
        switch (samples) {
                using enum ImageSamples;

            case One:
                return VK_SAMPLE_COUNT_1_BIT;

            case Two:
                return VK_SAMPLE_COUNT_2_BIT;

            case Four:
                return VK_SAMPLE_COUNT_4_BIT;

            case Eight:
                return VK_SAMPLE_COUNT_8_BIT;

            case Sixteen:
                return VK_SAMPLE_COUNT_16_BIT;

            case ThirtyTwo:
                return VK_SAMPLE_COUNT_32_BIT;

            case SixtyFour:
                return VK_SAMPLE_COUNT_64_BIT;
        }

        throw std::runtime_error("Unsupported sample count");
    }

    static constexpr VkQueueFlags toVkQueueFlags(const QueueFamilyFlags flags) {
        VkQueueFlags converted = 0;

        if (flags & QueueFamilyFlags::Graphics)
            converted |= VK_QUEUE_GRAPHICS_BIT;

        if (flags & QueueFamilyFlags::Transfer)
            converted |= VK_QUEUE_TRANSFER_BIT;

        if (flags & QueueFamilyFlags::Compute)
            converted |= VK_QUEUE_COMPUTE_BIT;

        return converted;
    }

    static constexpr VkShaderStageFlagBits toVkShaderStageFlag(const ShaderStage &stage) {
        switch (stage) {
            case ShaderStage::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;

            case ShaderStage::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;

            case ShaderStage::TesselationControl:
                return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

            case ShaderStage::TesselationEvaluation:
                return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

            case ShaderStage::Compute:
                return VK_SHADER_STAGE_COMPUTE_BIT;

            case ShaderStage::Geometry:
                return VK_SHADER_STAGE_GEOMETRY_BIT;
        }

        throw std::runtime_error("Unsupported shader stage");
    }

    static constexpr VkImageType toVkImageType(const ImageType &type) {
        switch (type) {
            case ImageType::OneD:
            case ImageType::OneDArray:
                return VK_IMAGE_TYPE_1D;

            case ImageType::TwoD:
            case ImageType::Cube:
            case ImageType::TwoDArray:
            case ImageType::CubeArray:
                return VK_IMAGE_TYPE_2D;

            case ImageType::ThreeD:
                return VK_IMAGE_TYPE_3D;
        }

        throw std::runtime_error("Unsupported image type");
    }

    [[maybe_unused]] static VkAttachmentLoadOp toVkLoadAction(const LoadAction loadAction) {
        switch (loadAction) {
                using enum LoadAction;

            case Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
                break;

            case Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
                break;

            case DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                break;
        }

        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    [[maybe_unused]] static VkAttachmentStoreOp toVkStoreAction(const StoreAction storeAction) {
        switch (storeAction) {
                using enum StoreAction;

            case Store:
                return VK_ATTACHMENT_STORE_OP_STORE;
                break;

            case Resolve:
                return VK_ATTACHMENT_STORE_OP_NONE;
                break;

            case StoreAndResolve:
                return VK_ATTACHMENT_STORE_OP_STORE;
                break;

            case DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
        }

        throw std::runtime_error("Unsupported store action");
    }

    static VkPipelineStageFlags2 toVkPipelineStages(const PipelineStageFlags flags) {
        VkPipelineStageFlags vkFlags = 0;

        if (flags & PipelineStageFlags::Top)
            vkFlags |= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

        if (flags & PipelineStageFlags::DrawIndirect)
            vkFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

        if (flags & PipelineStageFlags::VertexInput)
            vkFlags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;

        if (flags & PipelineStageFlags::VertexShader)
            vkFlags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

        if (flags & PipelineStageFlags::TessellationControl)
            vkFlags |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;

        if (flags & PipelineStageFlags::TessellationEvaluation)
            vkFlags |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;

        if (flags & PipelineStageFlags::GeometryShader)
            vkFlags |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;

        if (flags & PipelineStageFlags::FragmentShader)
            vkFlags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

        if (flags & PipelineStageFlags::EarlyFragmentTests)
            vkFlags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;

        if (flags & PipelineStageFlags::LateFragmentTests)
            vkFlags |= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        if (flags & PipelineStageFlags::ColorAttachmentOutput)
            vkFlags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        if (flags & PipelineStageFlags::ComputeShader)
            vkFlags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

        if (flags & PipelineStageFlags::Copy || flags & PipelineStageFlags::Resolve)
            vkFlags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;

        if (flags & PipelineStageFlags::Bottom)
            vkFlags |= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

        if (flags & PipelineStageFlags::AllGraphics)
            vkFlags |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        if (flags & PipelineStageFlags::AllCommands)
            vkFlags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        return vkFlags;
    }

    static VkAccessFlags toVkAccessFlags(const BarrierAccessFlags flags) {
        VkAccessFlags vkFlags = 0;

        if (flags & BarrierAccessFlags::IndirectCommandsRead)
            vkFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

        if (flags & BarrierAccessFlags::IndexRead)
            vkFlags |= VK_ACCESS_INDEX_READ_BIT;

        if (flags & BarrierAccessFlags::VertexAttributeRead)
            vkFlags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

        if (flags & BarrierAccessFlags::UniformRead)
            vkFlags |= VK_ACCESS_UNIFORM_READ_BIT;

        if (flags & BarrierAccessFlags::InputAttachmentRead)
            vkFlags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

        if (flags & BarrierAccessFlags::ShaderRead)
            vkFlags |= VK_ACCESS_SHADER_READ_BIT;

        if (flags & BarrierAccessFlags::ShaderWrite)
            vkFlags |= VK_ACCESS_SHADER_WRITE_BIT;

        if (flags & BarrierAccessFlags::ColorAttachmentRead)
            vkFlags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        if (flags & BarrierAccessFlags::ColorAttachmentWrite)
            vkFlags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (flags & BarrierAccessFlags::DepthStencilAttachmentRead)
            vkFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        if (flags & BarrierAccessFlags::DepthStencilAttachmentWrite)
            vkFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        if (flags & BarrierAccessFlags::CopyRead)
            vkFlags |= VK_ACCESS_TRANSFER_READ_BIT;

        if (flags & BarrierAccessFlags::CopyWrite)
            vkFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;

        if (flags & BarrierAccessFlags::HostRead)
            vkFlags |= VK_ACCESS_HOST_READ_BIT;

        if (flags & BarrierAccessFlags::HostWrite)
            vkFlags |= VK_ACCESS_HOST_WRITE_BIT;

        if (flags & BarrierAccessFlags::MemoryRead)
            vkFlags |= VK_ACCESS_MEMORY_READ_BIT;

        if (flags & BarrierAccessFlags::MemoryWrite)
            vkFlags |= VK_ACCESS_MEMORY_WRITE_BIT;

        if (flags & BarrierAccessFlags::FragmentShadingRateAttachmentRead)
            vkFlags |= VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;

        // TODO
        if (flags & BarrierAccessFlags::ResolveRead) {
        }
        if (flags & BarrierAccessFlags::ResolveWrite) {
        }
        if (flags & BarrierAccessFlags::StorageClear) {
        }

        return vkFlags;
    }

    static VkImageLayout toVkImageLayout(const ImageLayout layout) {
        switch (layout) {
                using enum ImageLayout;
            case Undefined:
                return VK_IMAGE_LAYOUT_UNDEFINED;
                break;

            case General:
                return VK_IMAGE_LAYOUT_GENERAL;
                break;

            case StorageOptimal:
                return VK_IMAGE_LAYOUT_GENERAL;
                break;

            case ColorAttachmentOptimal:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;

            case DepthStencilAttachmentOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                break;

            case DepthStencilReadOnlyOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                break;

            case ShaderReadOnlyOptimal:
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;

            case CopySourceOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                break;

            case CopyDestinationOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                break;

            case ResolveSourceOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                break;

            case ResolveDestinationOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                break;
        }

        throw std::invalid_argument("Unknown image layout");
    }

    static VkImageAspectFlags toVkImageAspectFlags(ImageAspectFlags flags) {
        VkImageAspectFlags vkFlags = 0;

        if (flags & ImageAspectFlags::Color)
            vkFlags |= VK_IMAGE_ASPECT_COLOR_BIT;

        if (flags & ImageAspectFlags::Depth)
            vkFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;

        if (flags & ImageAspectFlags::Stencil)
            vkFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;

        return vkFlags;
    }

    [[maybe_unused]] static std::string getVersionString(glm::ivec3 version) {
        return std::to_string(version.x) + "." +
               std::to_string(version.y) + "." +
               std::to_string(version.z);
    }

    [[maybe_unused]] static std::string getVersionString(const uint32_t version) {
        return std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
               std::to_string(VK_API_VERSION_MINOR(version)) + "." +
               std::to_string(VK_API_VERSION_PATCH(version));
    }

#ifdef DEBUG_ENABLED
    [[maybe_unused]] static VkBool32 vkDebugCallback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        const VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        [[maybe_unused]] void *pUserData
    ) {
        spdlog::level::level_enum level;
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                level = spdlog::level::debug;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                level = spdlog::level::info;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                level = spdlog::level::warn;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                level = spdlog::level::err;
                break;
            default:
                level = spdlog::level::warn;
                spdlog::warn("Unknown level flag in vkDebugCallback");
                break;
        }

        std::string source;
        switch (messageTypes) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                source = "Performance";
                break;

            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                source = "Validation";
                break;

            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                source = "General";
                break;

            default:
                source = string_VkDebugUtilsMessageTypeFlagsEXT(messageTypes);
                break;
        }

        auto vkDebugLogger = spdlog::get("Vulkan");
        if (vkDebugLogger == nullptr)
            vkDebugLogger = spdlog::stdout_color_mt("Vulkan");

        vkDebugLogger->log(
            level,
            "[{}] {}",
            source,
            pCallbackData->pMessage
        );

        return VK_FALSE;
    };
#endif
}
