#pragma once

namespace Vixen {
    struct Blending {
        enum class Operation {
            Add,
            Subtract,
            ReverseSubtract,
            Min,
            Max
        };

        enum class Factor {
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

        struct Mode {
            Factor sourceFactor;
            Factor destinationFactor;
            Operation operation;
        };

        bool colorBlendingEnabled;
        Mode color;
        bool alphaBlendingEnabled;
        Mode alpha;
    };
}
