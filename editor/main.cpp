#include "gl/Window.h"

int main() {
    auto window = Vixen::Engine::Gl::Window("Vixen Editor", 1080, 720);
    window.center();
    window.setVisible(true);

    while (!window.shouldClose()) {
        window.update();
    }
}
