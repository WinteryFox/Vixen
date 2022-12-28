#include <iostream>
#include "../engine/Window.h"

int main() {
    std::cout << "Henlo" <<  std::endl;
    auto window = Vixen::Editor::Window("Vixen Editor", 1080, 720);
    window.setVisible(true);

    while (!window.shouldClose())
        Vixen::Editor::Window::update();
}
