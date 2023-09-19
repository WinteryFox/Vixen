#include "../Vixen.h"
#include "GlWindow.h"
#include "GlRenderer.h"

namespace Vixen::Vk {
    class GlVixen : public Vixen {
        GlWindow window;

    public:
        GlVixen(const std::string &appTitle, const glm::vec3 &appVersion);
    };
}
