#include "../Vixen.h"
#include "GlWindow.h"
#include "GlRenderer.h"

namespace Vixen::Gl {
    class GlVixen : public Vixen {
        GlWindow window;

    public:
        GlVixen(const std::string &appTitle, const glm::vec3 &appVersion);
    };
}
