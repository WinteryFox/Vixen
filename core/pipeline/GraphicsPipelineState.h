#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "DynamicStateFlags.h"
#include "image/CompareOperator.h"
#include "image/ImageDataFormat.h"
#include "image/ImageSamples.h"
#include "rendering/ColorComponent.h"
#include "rendering/PrimitiveTopology.h"

namespace Vixen {
    enum class InputRate {
        Vertex,
        Instance
    };

    struct VertexBindingDescription {
        uint32_t binding;
        uint32_t stride;
        InputRate rate;
    };

    struct VertexAttributeDescription {
        uint32_t location;
        uint32_t binding;
        ImageDataFormat format;
        uint32_t offset;
    };

    enum class PolygonMode {
        Fill,
        Line,
        Point
    };

    enum class CullMode {
        None,
        Front,
        Back,
        FrontAndBack
    };

    enum class FrontFace {
        CounterClockwise,
        Clockwise
    };

    struct RasterizationState {
        bool isDepthClampEnabled = false;
        bool isRasterizerDiscardEnabled = false;
        PolygonMode polygonMode = PolygonMode::Fill;
        CullMode cullMode = CullMode::None;
        FrontFace frontFace = FrontFace::CounterClockwise;
        bool isDepthBiasEnabled = false;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasClamp = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float lineWidth = 1.0f;
    };

    struct MultisampleState {
        ImageSamples samples = ImageSamples::One;
        bool isSampleShadingEnabled = false;
        float minSampleShading = 1.0f;
        bool isAlphaToCoverageEnabled = false;
        bool isAlphaToOneEnabled = false;
    };

    enum class StencilOperator {
        Keep,
        Zero,
        Replace,
        IncrementAndClamp,
        DecrementAndClamp,
        Invert,
        IncrementAndWrap,
        DecrementAndWrap
    };

    struct StencilOperatorState {
        StencilOperator failOperator = StencilOperator::Keep;
        StencilOperator passOperator = StencilOperator::Keep;
        StencilOperator depthFailOperator = StencilOperator::Keep;
        CompareOperator compareOperator = CompareOperator::Always;
        uint32_t compareMask = ~uint32_t{0};
        uint32_t writeMask = ~uint32_t{0};
        uint32_t reference = 0;
    };

    struct DepthStencilState {
        bool isDepthTestEnabled = false;
        bool isDepthWriteEnabled = false;
        CompareOperator compareOperator = CompareOperator::Less;
        bool isDepthBoundsTestEnabled = false;
        bool isStencilTestEnabled = false;
        StencilOperatorState front{};
        StencilOperatorState back{};
        float minDepthBounds = 0.0f;
        float maxDepthBounds = 1.0f;
    };

    enum class BlendOperation {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class BlendFactor {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate
    };

    struct ColorBlendAttachmentState {
        bool isEnabled = false;

        BlendFactor sourceColorBlendFactor = BlendFactor::One;
        BlendFactor destinationColorBlendFactor = BlendFactor::Zero;
        BlendOperation colorBlendOperation = BlendOperation::Add;

        BlendFactor sourceAlphaBlendFactor = BlendFactor::One;
        BlendFactor destinationAlphaBlendFactor = BlendFactor::Zero;
        BlendOperation alphaBlendOperation = BlendOperation::Add;

        ColorComponentFlags colorWriteMask = ColorComponentBits::Red |
            ColorComponentBits::Green |
            ColorComponentBits::Blue |
            ColorComponentBits::Alpha;
    };

    struct GraphicsPipelineState {
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        bool isPrimitiveRestartEnabled = false;

        std::vector<VertexBindingDescription> vertexBindings;
        std::vector<VertexAttributeDescription> vertexAttributes;

        RasterizationState rasterization{};
        MultisampleState multisampling{};
        DepthStencilState depthStencil{};

        std::vector<ImageDataFormat> colorFormats;
        std::optional<ImageDataFormat> depthStencilFormat;
        std::vector<ColorBlendAttachmentState> colorBlending;
        std::array<float, 4> blendConstants{};

        DynamicStateFlags dynamicStates = DynamicStateBits::Viewport | DynamicStateBits::Scissor;
    };
}
