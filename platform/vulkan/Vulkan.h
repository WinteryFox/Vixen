#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <utility>

#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>

// WinBase.h exposes MemoryBarrier as a macro when Win32 Vulkan declarations are enabled.
// Vixen has a backend-independent MemoryBarrier type with the same name.
#ifdef MemoryBarrier
#undef MemoryBarrier
#endif
#include <glm/vec3.hpp>

#ifdef DEBUG_ENABLED
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include "core/BarrierAccessFlags.h"
#include "core/ImageDataFormat.h"
#include "core/IndexFormat.h"
#include "core/LoadAction.h"
#include "core/PipelineStageFlags.h"
#include "core/QueueFamilyFlags.h"
#include "core/StoreAction.h"
#include "core/error/ResourceCreationError.h"

#include "core/command/CommandBufferType.h"

#include "core/image/ImageAspectFlags.h"
#include "core/image/ImageLayout.h"
#include "core/image/ImageSamples.h"
#include "core/image/ImageSwizzle.h"
#include "core/image/ImageType.h"
#include "core/image/SamplerBorderColor.h"
#include "core/image/SamplerRepeatMode.h"

#include "core/shader/ShaderStage.h"
#include "core/shader/ShaderUniformType.h"

namespace Vixen {
    constexpr auto toVkComponentSwizzle(
        const ImageSwizzle swizzle
    ) -> std::expected<VkComponentSwizzle, ResourceCreationError> {
        switch (swizzle) {
            case ImageSwizzle::Identity:
                return VK_COMPONENT_SWIZZLE_IDENTITY;

            case ImageSwizzle::Zero:
                return VK_COMPONENT_SWIZZLE_ZERO;

            case ImageSwizzle::One:
                return VK_COMPONENT_SWIZZLE_ONE;

            case ImageSwizzle::Red:
                return VK_COMPONENT_SWIZZLE_R;

            case ImageSwizzle::Green:
                return VK_COMPONENT_SWIZZLE_G;

            case ImageSwizzle::Blue:
                return VK_COMPONENT_SWIZZLE_B;

            case ImageSwizzle::Alpha:
                return VK_COMPONENT_SWIZZLE_A;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Image component swizzle contains an unrecognized value"
            }
        };
    }

    constexpr auto toVkBorderColor(
        const SamplerBorderColor borderColor
    ) -> std::expected<VkBorderColor, ResourceCreationError> {
        switch (borderColor) {
            case SamplerBorderColor::FloatTransparentBlack:
                return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

            case SamplerBorderColor::IntTransparentBlack:
                return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;

            case SamplerBorderColor::FloatOpaqueBlack:
                return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

            case SamplerBorderColor::IntOpaqueBlack:
                return VK_BORDER_COLOR_INT_OPAQUE_BLACK;

            case SamplerBorderColor::FloatOpaqueWhite:
                return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

            case SamplerBorderColor::IntOpaqueWhite:
                return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Sampler border color contains an unrecognized value"
            }
        };
    }

    constexpr auto toVkSamplerAddressMode(
        const SamplerRepeatMode samplerRepeatMode
    ) -> std::expected<VkSamplerAddressMode, ResourceCreationError> {
        switch (samplerRepeatMode) {
            case SamplerRepeatMode::Repeat:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;

            case SamplerRepeatMode::MirroredRepeat:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

            case SamplerRepeatMode::ClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            case SamplerRepeatMode::ClampToBorder:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

            case SamplerRepeatMode::MirrorClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Sampler repeat mode contains an unrecognized value"
            }
        };
    }

    static constexpr auto toVkDataFormat(
        const ImageDataFormat format
    ) -> std::expected<VkFormat, ResourceCreationError> {
        switch (format) {
            case R4G4_UNORM_PACK8:
                return VK_FORMAT_R4G4_UNORM_PACK8;

            case R4G4B4A4_UNORM_PACK16:
                return VK_FORMAT_R4G4B4A4_UNORM_PACK16;

            case B4G4R4A4_UNORM_PACK16:
                return VK_FORMAT_B4G4R4A4_UNORM_PACK16;

            case R5G6B5_UNORM_PACK16:
                return VK_FORMAT_R5G6B5_UNORM_PACK16;

            case B5G6R5_UNORM_PACK16:
                return VK_FORMAT_B5G6R5_UNORM_PACK16;

            case R5G5B5A1_UNORM_PACK16:
                return VK_FORMAT_R5G5B5A1_UNORM_PACK16;

            case B5G5R5A1_UNORM_PACK16:
                return VK_FORMAT_B5G5R5A1_UNORM_PACK16;

            case A1R5G5B5_UNORM_PACK16:
                return VK_FORMAT_A1R5G5B5_UNORM_PACK16;

            case R8_UNORM:
                return VK_FORMAT_R8_UNORM;

            case R8_SNORM:
                return VK_FORMAT_R8_SNORM;

            case R8_USCALED:
                return VK_FORMAT_R8_USCALED;

            case R8_SSCALED:
                return VK_FORMAT_R8_SSCALED;

            case R8_UINT:
                return VK_FORMAT_R8_UINT;

            case R8_SINT:
                return VK_FORMAT_R8_SINT;

            case R8_SRGB:
                return VK_FORMAT_R8_SRGB;

            case R8G8_UNORM:
                return VK_FORMAT_R8G8_UNORM;

            case R8G8_SNORM:
                return VK_FORMAT_R8G8_SNORM;

            case R8G8_USCALED:
                return VK_FORMAT_R8G8_USCALED;

            case R8G8_SSCALED:
                return VK_FORMAT_R8G8_SSCALED;

            case R8G8_UINT:
                return VK_FORMAT_R8G8_UINT;

            case R8G8_SINT:
                return VK_FORMAT_R8G8_SINT;

            case R8G8_SRGB:
                return VK_FORMAT_R8G8_SRGB;

            case R8G8B8_UNORM:
                return VK_FORMAT_R8G8B8_UNORM;

            case R8G8B8_SNORM:
                return VK_FORMAT_R8G8B8_SNORM;

            case R8G8B8_USCALED:
                return VK_FORMAT_R8G8B8_USCALED;

            case R8G8B8_SSCALED:
                return VK_FORMAT_R8G8B8_SSCALED;

            case R8G8B8_UINT:
                return VK_FORMAT_R8G8B8_UINT;

            case R8G8B8_SINT:
                return VK_FORMAT_R8G8B8_SINT;

            case R8G8B8_SRGB:
                return VK_FORMAT_R8G8B8_SRGB;

            case B8G8R8_UNORM:
                return VK_FORMAT_B8G8R8_UNORM;

            case B8G8R8_SNORM:
                return VK_FORMAT_B8G8R8_SNORM;

            case B8G8R8_USCALED:
                return VK_FORMAT_B8G8R8_USCALED;

            case B8G8R8_SSCALED:
                return VK_FORMAT_B8G8R8_SSCALED;

            case B8G8R8_UINT:
                return VK_FORMAT_B8G8R8_UINT;

            case B8G8R8_SINT:
                return VK_FORMAT_B8G8R8_SINT;

            case B8G8R8_SRGB:
                return VK_FORMAT_B8G8R8_SRGB;

            case R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;

            case R8G8B8A8_SNORM:
                return VK_FORMAT_R8G8B8A8_SNORM;

            case R8G8B8A8_USCALED:
                return VK_FORMAT_R8G8B8A8_USCALED;

            case R8G8B8A8_SSCALED:
                return VK_FORMAT_R8G8B8A8_SSCALED;

            case R8G8B8A8_UINT:
                return VK_FORMAT_R8G8B8A8_UINT;

            case R8G8B8A8_SINT:
                return VK_FORMAT_R8G8B8A8_SINT;

            case R8G8B8A8_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;

            case B8G8R8A8_UNORM:
                return VK_FORMAT_B8G8R8A8_UNORM;

            case B8G8R8A8_SNORM:
                return VK_FORMAT_B8G8R8A8_SNORM;

            case B8G8R8A8_USCALED:
                return VK_FORMAT_B8G8R8A8_USCALED;

            case B8G8R8A8_SSCALED:
                return VK_FORMAT_B8G8R8A8_SSCALED;

            case B8G8R8A8_UINT:
                return VK_FORMAT_B8G8R8A8_UINT;

            case B8G8R8A8_SINT:
                return VK_FORMAT_B8G8R8A8_SINT;

            case B8G8R8A8_SRGB:
                return VK_FORMAT_B8G8R8A8_SRGB;

            case A8B8G8R8_UNORM_PACK32:
                return VK_FORMAT_A8B8G8R8_UNORM_PACK32;

            case A8B8G8R8_SNORM_PACK32:
                return VK_FORMAT_A8B8G8R8_SNORM_PACK32;

            case A8B8G8R8_USCALED_PACK32:
                return VK_FORMAT_A8B8G8R8_USCALED_PACK32;

            case A8B8G8R8_SSCALED_PACK32:
                return VK_FORMAT_A8B8G8R8_SSCALED_PACK32;

            case A8B8G8R8_UINT_PACK32:
                return VK_FORMAT_A8B8G8R8_UINT_PACK32;

            case A8B8G8R8_SINT_PACK32:
                return VK_FORMAT_A8B8G8R8_SINT_PACK32;

            case A8B8G8R8_SRGB_PACK32:
                return VK_FORMAT_A8B8G8R8_SRGB_PACK32;

            case A2R10G10B10_UNORM_PACK32:
                return VK_FORMAT_A2R10G10B10_UNORM_PACK32;

            case A2R10G10B10_SNORM_PACK32:
                return VK_FORMAT_A2R10G10B10_SNORM_PACK32;

            case A2R10G10B10_USCALED_PACK32:
                return VK_FORMAT_A2R10G10B10_USCALED_PACK32;

            case A2R10G10B10_SSCALED_PACK32:
                return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;

            case A2R10G10B10_UINT_PACK32:
                return VK_FORMAT_A2R10G10B10_UINT_PACK32;

            case A2R10G10B10_SINT_PACK32:
                return VK_FORMAT_A2R10G10B10_SINT_PACK32;

            case A2B10G10R10_UNORM_PACK32:
                return VK_FORMAT_A2B10G10R10_UNORM_PACK32;

            case A2B10G10R10_SNORM_PACK32:
                return VK_FORMAT_A2B10G10R10_SNORM_PACK32;

            case A2B10G10R10_USCALED_PACK32:
                return VK_FORMAT_A2B10G10R10_USCALED_PACK32;

            case A2B10G10R10_SSCALED_PACK32:
                return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;

            case A2B10G10R10_UINT_PACK32:
                return VK_FORMAT_A2B10G10R10_UINT_PACK32;

            case A2B10G10R10_SINT_PACK32:
                return VK_FORMAT_A2B10G10R10_SINT_PACK32;

            case R16_UNORM:
                return VK_FORMAT_R16_UNORM;

            case R16_SNORM:
                return VK_FORMAT_R16_SNORM;

            case R16_USCALED:
                return VK_FORMAT_R16_USCALED;

            case R16_SSCALED:
                return VK_FORMAT_R16_SSCALED;

            case R16_UINT:
                return VK_FORMAT_R16_UINT;

            case R16_SINT:
                return VK_FORMAT_R16_SINT;

            case R16_SFLOAT:
                return VK_FORMAT_R16_SFLOAT;

            case R16G16_UNORM:
                return VK_FORMAT_R16G16_UNORM;

            case R16G16_SNORM:
                return VK_FORMAT_R16G16_SNORM;

            case R16G16_USCALED:
                return VK_FORMAT_R16G16_USCALED;

            case R16G16_SSCALED:
                return VK_FORMAT_R16G16_SSCALED;

            case R16G16_UINT:
                return VK_FORMAT_R16G16_UINT;

            case R16G16_SINT:
                return VK_FORMAT_R16G16_SINT;

            case R16G16_SFLOAT:
                return VK_FORMAT_R16G16_SFLOAT;

            case R16G16B16_UNORM:
                return VK_FORMAT_R16G16B16_UNORM;

            case R16G16B16_SNORM:
                return VK_FORMAT_R16G16B16_SNORM;

            case R16G16B16_USCALED:
                return VK_FORMAT_R16G16B16_USCALED;

            case R16G16B16_SSCALED:
                return VK_FORMAT_R16G16B16_SSCALED;

            case R16G16B16_UINT:
                return VK_FORMAT_R16G16B16_UINT;

            case R16G16B16_SINT:
                return VK_FORMAT_R16G16B16_SINT;

            case R16G16B16_SFLOAT:
                return VK_FORMAT_R16G16B16_SFLOAT;

            case R16G16B16A16_UNORM:
                return VK_FORMAT_R16G16B16A16_UNORM;

            case R16G16B16A16_SNORM:
                return VK_FORMAT_R16G16B16A16_SNORM;

            case R16G16B16A16_USCALED:
                return VK_FORMAT_R16G16B16A16_USCALED;

            case R16G16B16A16_SSCALED:
                return VK_FORMAT_R16G16B16A16_SSCALED;

            case R16G16B16A16_UINT:
                return VK_FORMAT_R16G16B16A16_UINT;

            case R16G16B16A16_SINT:
                return VK_FORMAT_R16G16B16A16_SINT;

            case R16G16B16A16_SFLOAT:
                return VK_FORMAT_R16G16B16A16_SFLOAT;

            case R32_UINT:
                return VK_FORMAT_R32_UINT;

            case R32_SINT:
                return VK_FORMAT_R32_SINT;

            case R32_SFLOAT:
                return VK_FORMAT_R32_SFLOAT;

            case R32G32_UINT:
                return VK_FORMAT_R32G32_UINT;

            case R32G32_SINT:
                return VK_FORMAT_R32G32_SINT;

            case R32G32_SFLOAT:
                return VK_FORMAT_R32G32_SFLOAT;

            case R32G32B32_UINT:
                return VK_FORMAT_R32G32B32_UINT;

            case R32G32B32_SINT:
                return VK_FORMAT_R32G32B32_SINT;

            case R32G32B32_SFLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;

            case R32G32B32A32_UINT:
                return VK_FORMAT_R32G32B32A32_UINT;

            case R32G32B32A32_SINT:
                return VK_FORMAT_R32G32B32A32_SINT;

            case R32G32B32A32_SFLOAT:
                return VK_FORMAT_R32G32B32A32_SFLOAT;

            case R64_UINT:
                return VK_FORMAT_R64_UINT;

            case R64_SINT:
                return VK_FORMAT_R64_SINT;

            case R64_SFLOAT:
                return VK_FORMAT_R64_SFLOAT;

            case R64G64_UINT:
                return VK_FORMAT_R64G64_UINT;

            case R64G64_SINT:
                return VK_FORMAT_R64G64_SINT;

            case R64G64_SFLOAT:
                return VK_FORMAT_R64G64_SFLOAT;

            case R64G64B64_UINT:
                return VK_FORMAT_R64G64B64_UINT;

            case R64G64B64_SINT:
                return VK_FORMAT_R64G64B64_SINT;

            case R64G64B64_SFLOAT:
                return VK_FORMAT_R64G64B64_SFLOAT;

            case R64G64B64A64_UINT:
                return VK_FORMAT_R64G64B64A64_UINT;

            case R64G64B64A64_SINT:
                return VK_FORMAT_R64G64B64A64_SINT;

            case R64G64B64A64_SFLOAT:
                return VK_FORMAT_R64G64B64A64_SFLOAT;

            case B10G11R11_UFLOAT_PACK32:
                return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

            case E5B9G9R9_UFLOAT_PACK32:
                return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

            case D16_UNORM:
                return VK_FORMAT_D16_UNORM;

            case X8_D24_UNORM_PACK32:
                return VK_FORMAT_X8_D24_UNORM_PACK32;

            case D32_SFLOAT:
                return VK_FORMAT_D32_SFLOAT;

            case S8_UINT:
                return VK_FORMAT_S8_UINT;

            case D16_UNORM_S8_UINT:
                return VK_FORMAT_D16_UNORM_S8_UINT;

            case D24_UNORM_S8_UINT:
                return VK_FORMAT_D24_UNORM_S8_UINT;

            case D32_SFLOAT_S8_UINT:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;

            case BC1_RGB_UNORM_BLOCK:
                return VK_FORMAT_BC1_RGB_UNORM_BLOCK;

            case BC1_RGB_SRGB_BLOCK:
                return VK_FORMAT_BC1_RGB_SRGB_BLOCK;

            case BC1_RGBA_UNORM_BLOCK:
                return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

            case BC1_RGBA_SRGB_BLOCK:
                return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

            case BC2_UNORM_BLOCK:
                return VK_FORMAT_BC2_UNORM_BLOCK;

            case BC2_SRGB_BLOCK:
                return VK_FORMAT_BC2_SRGB_BLOCK;

            case BC3_UNORM_BLOCK:
                return VK_FORMAT_BC3_UNORM_BLOCK;

            case BC3_SRGB_BLOCK:
                return VK_FORMAT_BC3_SRGB_BLOCK;

            case BC4_UNORM_BLOCK:
                return VK_FORMAT_BC4_UNORM_BLOCK;

            case BC4_SNORM_BLOCK:
                return VK_FORMAT_BC4_SNORM_BLOCK;

            case BC5_UNORM_BLOCK:
                return VK_FORMAT_BC5_UNORM_BLOCK;

            case BC5_SNORM_BLOCK:
                return VK_FORMAT_BC5_SNORM_BLOCK;

            case BC6H_UFLOAT_BLOCK:
                return VK_FORMAT_BC6H_UFLOAT_BLOCK;

            case BC6H_SFLOAT_BLOCK:
                return VK_FORMAT_BC6H_SFLOAT_BLOCK;

            case BC7_UNORM_BLOCK:
                return VK_FORMAT_BC7_UNORM_BLOCK;

            case BC7_SRGB_BLOCK:
                return VK_FORMAT_BC7_SRGB_BLOCK;

            case ETC2_R8G8B8_UNORM_BLOCK:
                return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;

            case ETC2_R8G8B8_SRGB_BLOCK:
                return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;

            case ETC2_R8G8B8A1_UNORM_BLOCK:
                return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;

            case ETC2_R8G8B8A1_SRGB_BLOCK:
                return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;

            case ETC2_R8G8B8A8_UNORM_BLOCK:
                return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;

            case ETC2_R8G8B8A8_SRGB_BLOCK:
                return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;

            case EAC_R11_UNORM_BLOCK:
                return VK_FORMAT_EAC_R11_UNORM_BLOCK;

            case EAC_R11_SNORM_BLOCK:
                return VK_FORMAT_EAC_R11_SNORM_BLOCK;

            case EAC_R11G11_UNORM_BLOCK:
                return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;

            case EAC_R11G11_SNORM_BLOCK:
                return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;

            case ASTC_4x4_UNORM_BLOCK:
                return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;

            case ASTC_4x4_SRGB_BLOCK:
                return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;

            case ASTC_5x4_UNORM_BLOCK:
                return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;

            case ASTC_5x4_SRGB_BLOCK:
                return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;

            case ASTC_5x5_UNORM_BLOCK:
                return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;

            case ASTC_5x5_SRGB_BLOCK:
                return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;

            case ASTC_6x5_UNORM_BLOCK:
                return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;

            case ASTC_6x5_SRGB_BLOCK:
                return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;

            case ASTC_6x6_UNORM_BLOCK:
                return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;

            case ASTC_6x6_SRGB_BLOCK:
                return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;

            case ASTC_8x5_UNORM_BLOCK:
                return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;

            case ASTC_8x5_SRGB_BLOCK:
                return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;

            case ASTC_8x6_UNORM_BLOCK:
                return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;

            case ASTC_8x6_SRGB_BLOCK:
                return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;

            case ASTC_8x8_UNORM_BLOCK:
                return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;

            case ASTC_8x8_SRGB_BLOCK:
                return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;

            case ASTC_10x5_UNORM_BLOCK:
                return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;

            case ASTC_10x5_SRGB_BLOCK:
                return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;

            case ASTC_10x6_UNORM_BLOCK:
                return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;

            case ASTC_10x6_SRGB_BLOCK:
                return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;

            case ASTC_10x8_UNORM_BLOCK:
                return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;

            case ASTC_10x8_SRGB_BLOCK:
                return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;

            case ASTC_10x10_UNORM_BLOCK:
                return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;

            case ASTC_10x10_SRGB_BLOCK:
                return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;

            case ASTC_12x10_UNORM_BLOCK:
                return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;

            case ASTC_12x10_SRGB_BLOCK:
                return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;

            case ASTC_12x12_UNORM_BLOCK:
                return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;

            case ASTC_12x12_SRGB_BLOCK:
                return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;

            case G8B8G8R8_422_UNORM:
                return VK_FORMAT_G8B8G8R8_422_UNORM;

            case B8G8R8G8_422_UNORM:
                return VK_FORMAT_B8G8R8G8_422_UNORM;

            case G8_B8_R8_3PLANE_420_UNORM:
                return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;

            case G8_B8R8_2PLANE_420_UNORM:
                return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;

            case G8_B8_R8_3PLANE_422_UNORM:
                return VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;

            case G8_B8R8_2PLANE_422_UNORM:
                return VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;

            case G8_B8_R8_3PLANE_444_UNORM:
                return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;

            case R10X6_UNORM_PACK16:
                return VK_FORMAT_R10X6_UNORM_PACK16;

            case R10X6G10X6_UNORM_2PACK16:
                return VK_FORMAT_R10X6G10X6_UNORM_2PACK16;

            case R10X6G10X6B10X6A10X6_UNORM_4PACK16:
                return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;

            case G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
                return VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;

            case B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
                return VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;

            case G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
                return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;

            case G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
                return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;

            case G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
                return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;

            case G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
                return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;

            case G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
                return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;

            case R12X4_UNORM_PACK16:
                return VK_FORMAT_R12X4_UNORM_PACK16;

            case R12X4G12X4_UNORM_2PACK16:
                return VK_FORMAT_R12X4G12X4_UNORM_2PACK16;

            case R12X4G12X4B12X4A12X4_UNORM_4PACK16:
                return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;

            case G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
                return VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;

            case B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
                return VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;

            case G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
                return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;

            case G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
                return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;

            case G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
                return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;

            case G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
                return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;

            case G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
                return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;

            case G16B16G16R16_422_UNORM:
                return VK_FORMAT_G16B16G16R16_422_UNORM;

            case B16G16R16G16_422_UNORM:
                return VK_FORMAT_B16G16R16G16_422_UNORM;

            case G16_B16_R16_3PLANE_420_UNORM:
                return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;

            case G16_B16R16_2PLANE_420_UNORM:
                return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;

            case G16_B16_R16_3PLANE_422_UNORM:
                return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;

            case G16_B16R16_2PLANE_422_UNORM:
                return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;

            case G16_B16_R16_3PLANE_444_UNORM:
                return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Image data format contains an unrecognized value"
            }
        };
    }

    static constexpr auto toVkCommandBufferLevel(
        const CommandBufferType type
    ) -> std::expected<VkCommandBufferLevel, ResourceCreationError> {
        switch (type) {
            case CommandBufferType::Primary:
                return VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            case CommandBufferType::Secondary:
                return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Command-buffer type contains an unrecognized value"
            }
        };
    }

    static constexpr auto toVkIndexType(const IndexFormat format) -> std::expected<VkIndexType, ResourceCreationError> {
        switch (format) {
            case IndexFormat::UnsignedInt16:
                return VK_INDEX_TYPE_UINT16;

            case IndexFormat::UnsignedInt32:
                return VK_INDEX_TYPE_UINT32;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Index format contains an unrecognized value"
            }
        };
    }

    static constexpr auto toVkSampleCountFlagBits(
        const ImageSamples& samples
    ) -> std::expected<VkSampleCountFlagBits, ResourceCreationError> {
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

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Image sample count contains an unrecognized value"
            }
        };
    }

    static constexpr VkQueueFlags toVkQueueFlags(const QueueFamilyFlags flags) {
        VkQueueFlags converted = 0;

        if (flags.contains(QueueFamilyBits::Graphics))
            converted |= VK_QUEUE_GRAPHICS_BIT;

        if (flags.contains(QueueFamilyBits::Transfer))
            converted |= VK_QUEUE_TRANSFER_BIT;

        if (flags.contains(QueueFamilyBits::Compute))
            converted |= VK_QUEUE_COMPUTE_BIT;

        return converted;
    }

    static constexpr VkShaderStageFlags toVkShaderStageFlags(const ShaderStageFlags& stage) {
        VkShaderStageFlags stages = 0;

        if (stage.contains(ShaderStageBits::Vertex))
            stages |= VK_SHADER_STAGE_VERTEX_BIT;

        if (stage.contains(ShaderStageBits::Fragment))
            stages |= VK_SHADER_STAGE_FRAGMENT_BIT;

        if (stage.contains(ShaderStageBits::TesselationControl))
            stages |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

        if (stage.contains(ShaderStageBits::TesselationEvaluation))
            stages |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        if (stage.contains(ShaderStageBits::Compute))
            stages |= VK_SHADER_STAGE_COMPUTE_BIT;

        if (stage.contains(ShaderStageBits::Geometry))
            stages |= VK_SHADER_STAGE_GEOMETRY_BIT;

        return stages;
    }

    static constexpr auto toVkDescriptorType(
        const ShaderUniformType type) -> std::expected<VkDescriptorType, ResourceCreationError> {
        switch (type) {
            case ShaderUniformType::Sampler:
                return VK_DESCRIPTOR_TYPE_SAMPLER;

            case ShaderUniformType::SampledImage:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

            case ShaderUniformType::CombinedImageSampler:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            case ShaderUniformType::StorageImage:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

            case ShaderUniformType::UniformBuffer:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            case ShaderUniformType::StorageBuffer:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            case ShaderUniformType::UniformTexelBuffer:
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

            case ShaderUniformType::StorageTexelBuffer:
                return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

            case ShaderUniformType::InputAttachment:
                return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Shader uniform type does not map to a Vulkan descriptor type"
            }
        };
    }

    static constexpr auto toVkImageType(const ImageType& type) -> std::expected<VkImageType, ResourceCreationError> {
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

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Image type contains an unrecognized value"
            }
        };
    }

    static constexpr auto toVkImageViewType(
        const ImageType& type
    ) -> std::expected<VkImageViewType, ResourceCreationError> {
        switch (type) {
            case ImageType::OneD:
                return VK_IMAGE_VIEW_TYPE_1D;

            case ImageType::TwoD:
                return VK_IMAGE_VIEW_TYPE_2D;

            case ImageType::ThreeD:
                return VK_IMAGE_VIEW_TYPE_3D;

            case ImageType::Cube:
                return VK_IMAGE_VIEW_TYPE_CUBE;

            case ImageType::OneDArray:
                return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

            case ImageType::TwoDArray:
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

            case ImageType::CubeArray:
                return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Image type does not map to a Vulkan image-view type"
            }
        };
    }

    [[maybe_unused]] static constexpr auto toVkLoadAction(
        const LoadAction loadAction
    ) -> std::expected<VkAttachmentLoadOp, ResourceCreationError> {
        switch (loadAction) {
                using enum LoadAction;

            case Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;

            case Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;

            case DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Attachment load action contains an unrecognized value"
            }
        };
    }

    [[maybe_unused]] static constexpr auto toVkStoreAction(
        const StoreAction storeAction
    ) -> std::expected<VkAttachmentStoreOp, ResourceCreationError> {
        switch (storeAction) {
                using enum StoreAction;

            case Store:
                return VK_ATTACHMENT_STORE_OP_STORE;

            case Resolve:
                return VK_ATTACHMENT_STORE_OP_NONE;

            case StoreAndResolve:
                return VK_ATTACHMENT_STORE_OP_STORE;

            case DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Attachment store action contains an unrecognized value"
            }
        };
    }

    static constexpr VkPipelineStageFlags2 toVkPipelineStages(const PipelineStageFlags flags) {
        VkPipelineStageFlags2 vkFlags = 0;

        if (flags.contains(PipelineStageBits::Top))
            vkFlags |= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

        if (flags.contains(PipelineStageBits::DrawIndirect))
            vkFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

        if (flags.contains(PipelineStageBits::VertexInput))
            vkFlags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;

        if (flags.contains(PipelineStageBits::VertexShader))
            vkFlags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

        if (flags.contains(PipelineStageBits::TessellationControl))
            vkFlags |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;

        if (flags.contains(PipelineStageBits::TessellationEvaluation))
            vkFlags |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;

        if (flags.contains(PipelineStageBits::GeometryShader))
            vkFlags |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;

        if (flags.contains(PipelineStageBits::FragmentShader))
            vkFlags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

        if (flags.contains(PipelineStageBits::EarlyFragmentTests))
            vkFlags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;

        if (flags.contains(PipelineStageBits::LateFragmentTests))
            vkFlags |= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        if (flags.contains(PipelineStageBits::ColorAttachmentOutput))
            vkFlags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        if (flags.contains(PipelineStageBits::ComputeShader))
            vkFlags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

        if (flags.contains(PipelineStageBits::Copy))
            vkFlags |= VK_PIPELINE_STAGE_2_COPY_BIT;

        if (flags.contains(PipelineStageBits::Resolve))
            vkFlags |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;

        if (flags.contains(PipelineStageBits::Bottom))
            vkFlags |= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

        if (flags.contains(PipelineStageBits::AllGraphics))
            vkFlags |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        if (flags.contains(PipelineStageBits::AllCommands))
            vkFlags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        return vkFlags;
    }

    static constexpr VkAccessFlags2 toVkAccessFlags(const BarrierAccessFlags flags) {
        VkAccessFlags2 vkFlags = 0;

        if (flags.contains(BarrierAccessBits::IndirectCommandsRead))
            vkFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

        if (flags.contains(BarrierAccessBits::IndexRead))
            vkFlags |= VK_ACCESS_INDEX_READ_BIT;

        if (flags.contains(BarrierAccessBits::VertexAttributeRead))
            vkFlags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

        if (flags.contains(BarrierAccessBits::UniformRead))
            vkFlags |= VK_ACCESS_UNIFORM_READ_BIT;

        if (flags.contains(BarrierAccessBits::InputAttachmentRead))
            vkFlags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

        if (flags.contains(BarrierAccessBits::ShaderRead))
            vkFlags |= VK_ACCESS_SHADER_READ_BIT;

        if (flags.contains(BarrierAccessBits::ShaderWrite))
            vkFlags |= VK_ACCESS_SHADER_WRITE_BIT;

        if (flags.contains(BarrierAccessBits::ColorAttachmentRead))
            vkFlags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        if (flags.contains(BarrierAccessBits::ColorAttachmentWrite))
            vkFlags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (flags.contains(BarrierAccessBits::DepthStencilAttachmentRead))
            vkFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        if (flags.contains(BarrierAccessBits::DepthStencilAttachmentWrite))
            vkFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        if (flags.contains(BarrierAccessBits::CopyRead))
            vkFlags |= VK_ACCESS_TRANSFER_READ_BIT;

        if (flags.contains(BarrierAccessBits::CopyWrite))
            vkFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;

        if (flags.contains(BarrierAccessBits::HostRead))
            vkFlags |= VK_ACCESS_HOST_READ_BIT;

        if (flags.contains(BarrierAccessBits::HostWrite))
            vkFlags |= VK_ACCESS_HOST_WRITE_BIT;

        if (flags.contains(BarrierAccessBits::MemoryRead))
            vkFlags |= VK_ACCESS_MEMORY_READ_BIT;

        if (flags.contains(BarrierAccessBits::MemoryWrite))
            vkFlags |= VK_ACCESS_MEMORY_WRITE_BIT;

        if (flags.contains(BarrierAccessBits::FragmentShadingRateAttachmentRead))
            vkFlags |= VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;

        // TODO
        if (flags.contains(BarrierAccessBits::ResolveRead))
            vkFlags |= VK_ACCESS_TRANSFER_READ_BIT;

        if (flags.contains(BarrierAccessBits::ResolveWrite))
            vkFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;

        if (flags.contains(BarrierAccessBits::StorageClear))
            vkFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;

        return vkFlags;
    }

    static constexpr auto toVkImageLayout(
        const ImageLayout layout
    ) -> std::expected<VkImageLayout, ResourceCreationError> {
        switch (layout) {
                using enum ImageLayout;
            case Undefined:
                return VK_IMAGE_LAYOUT_UNDEFINED;

            case General:
            case StorageOptimal:
                return VK_IMAGE_LAYOUT_GENERAL;

            case ColorAttachmentOptimal:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            case DepthStencilAttachmentOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            case DepthStencilReadOnlyOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            case ShaderReadOnlyOptimal:
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            case CopySourceOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            case CopyDestinationOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            case ResolveSourceOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            case ResolveDestinationOptimal:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }

        return std::unexpected{
            ResourceCreationError{
                .code = ResourceCreationErrorCode::InvalidDescription,
                .message = "Image layout contains an unrecognized value"
            }
        };
    }

    static constexpr VkImageAspectFlags toVkImageAspectFlags(const ImageAspectFlags flags) {
        VkImageAspectFlags vkFlags = 0;

        if (flags.contains(ImageAspectBits::Color))
            vkFlags |= VK_IMAGE_ASPECT_COLOR_BIT;

        if (flags.contains(ImageAspectBits::Depth))
            vkFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;

        if (flags.contains(ImageAspectBits::Stencil))
            vkFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;

        return vkFlags;
    }

    [[maybe_unused]] static constexpr std::string getVersionString(const glm::uvec3 version) {
        return std::to_string(version.x) + "." + std::to_string(version.y) + "." + std::to_string(version.z);
    }

    [[maybe_unused]] static constexpr std::string getVersionString(const uint32_t version) {
        return std::to_string(VK_API_VERSION_MAJOR(version)) + "." + std::to_string(VK_API_VERSION_MINOR(version)) +
            "." + std::to_string(VK_API_VERSION_PATCH(version));
    }

    #ifdef DEBUG_ENABLED
    [[maybe_unused]] static constexpr VkBool32
    vkDebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    const VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData) {
        spdlog::level::level_enum level{};
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

            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
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

        vkDebugLogger->log(level, "[{}] {}", source, pCallbackData->pMessage);

        return VK_FALSE;
    };
    #endif
} // namespace Vixen
