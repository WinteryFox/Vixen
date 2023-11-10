#include "GlVixen.h"

namespace Vixen::Gl {
    GlVixen::GlVixen(const std::string &appTitle, const glm::vec3 &appVersion)
            : Vixen(appTitle, appVersion),
              window(GlWindow(appTitle, 640, 480, false)) {
        window.center();
        window.maximize();
        window.setVisible(true);
    }
}
