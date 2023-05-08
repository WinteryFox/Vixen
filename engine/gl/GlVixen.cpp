#include "GlVixen.h"

namespace Vixen::Engine {
    GlVixen::GlVixen(const std::string &appTitle, const glm::vec3 &appVersion)
            : Vixen(appTitle, appVersion, std::make_shared<GlRenderer>()),
              window(GlWindow(appTitle, 720, 480, false)) {
        window.center();
        window.setVisible(true);
    }
}
