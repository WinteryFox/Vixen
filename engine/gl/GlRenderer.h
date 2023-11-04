#include <cstdint>
#include "../Renderer.h"

namespace Vixen::Gl {
    class GlRenderer : public Renderer {
        struct DrawElementsIndirectCommand {
            std::uint32_t count;
            std::uint32_t instanceCount;
            std::uint32_t firstIndex;
            std::int32_t baseVertex;
            std::uint32_t baseInstance;
        };

    public:
        GlRenderer();

        void submit() override;
    };
}
