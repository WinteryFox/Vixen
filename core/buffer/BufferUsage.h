#pragma once

namespace Vixen {
    enum BufferUsage {
        BUFFER_USAGE_COPY_SOURCE,
        BUFFER_USAGE_COPY_DESTINATION,
        BUFFER_USAGE_TEXEL,
        BUFFER_USAGE_UNIFORM,
        BUFFER_USAGE_STORAGE,
        BUFFER_USAGE_VERTEX,
        BUFFER_USAGE_INDEX,
        BUFFER_USAGE_INDIRECT
    };
}
