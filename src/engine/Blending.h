#pragma once

namespace Vixen {
    struct Blending {
        enum class Operation {
            ADD,
            SUBTRACT,
            REVERSE_SUBTRACT,
            MIN,
            MAX,
        };

        enum class Factor {
            ZERO,
            ONE,
            SRC_COLOR,
            ONE_MINUS_SRC_COLOR,
            DST_COLOR,
            ONE_MINUS_DST_COLOR,
            SRC_ALPHA,
            ONE_MINUS_SRC_ALPHA,
            DST_ALPHA,
            ONE_MINUS_DST_ALPHA,
            CONSTANT_COLOR,
            ONE_MINUS_CONSTANT_COLOR,
            CONSTANT_ALPHA,
            ONE_MINUS_CONSTANT_ALPHA,
            SRC_ALPHA_SATURATE,
            SRC1_COLOR,
            ONE_MINUS_SRC1_COLOR,
            SRC1_ALPHA,
            ONE_MINUS_SRC1_ALPHA,
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