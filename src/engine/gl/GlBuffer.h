#pragma once

#include <GL/glew.h>
#include <spdlog/spdlog.h>
#include "../Buffer.h"
#include "../Util.h"

namespace Vixen::Gl {
    class GlBuffer : public Buffer {
    private:
        GLuint buffer = 0;

        GLbitfield flags;

    public:
        GlBuffer(Usage bufferUsage, const size_t &size);

        ~GlBuffer();

        void write(const char *data, size_t dataSize, size_t offset) override;

        char *map() override;

        void unmap() override;

        [[nodiscard]] GLuint getBuffer() const;
    };
}
