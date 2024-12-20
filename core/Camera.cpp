#include "Camera.h"

#include <cmath>

namespace Vixen {
    Camera::Camera(
        const glm::vec3 position,
        const glm::vec3 rotation,
        const float fieldOfView,
        const float nearPlane,
        const float farPlane
    ) : lastX(0),
        lastY(0),
        position(position),
        rotation(rotation),
        fieldOfView(fieldOfView),
        nearPlane(nearPlane),
        farPlane(farPlane) {}

    void Camera::update(GLFWwindow* window, const double deltaTime) {
        double currentX = 0;
        double currentY = 0;
        glfwGetCursorPos(window, &currentX, &currentY);

        const double xOffset = currentX - lastX;
        const double yOffset = currentY - lastY;
        lastX = currentX;
        lastY = currentY;

        constexpr double sensitivity = 1.0F;
        rotation.y += static_cast<float>(sensitivity * xOffset * deltaTime);
        rotation.x += static_cast<float>(sensitivity * yOffset * deltaTime);

        const auto& direction = normalize(glm::vec3(
            std::cos(rotation.x) * std::sin(rotation.y),
            std::sin(rotation.x),
            std::cos(rotation.x) * std::cos(rotation.y)
        ));
        const auto& right = normalize(glm::vec3(
            sin(rotation.y - std::numbers::pi / 2.0f),
            0.0f,
            cos(rotation.y - std::numbers::pi / 2.0f)
        ));

        auto advance = glm::vec3(0.0f);

        constexpr float speed = 10.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            advance += direction * speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            advance -= direction * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            advance += right * speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            advance -= right * speed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            advance += glm::vec3(0, 1, 0) * speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            advance -= glm::vec3(0, 1, 0) * speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            advance *= 2.0f;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        advance *= deltaTime;
        position += advance;
    }


    /**
     * \brief Calculates the view matrix for this camera.
     * \return Returns the view matrix.
     */
    glm::mat4 Camera::view() const {
        const auto& direction = normalize(glm::vec3(
            std::cos(rotation.x) * std::sin(rotation.y),
            std::sin(rotation.x),
            std::cos(rotation.x) * std::cos(rotation.y)
        ));
        const auto& right = normalize(glm::vec3(
            sin(rotation.y - std::numbers::pi / 2.0f),
            0.0f,
            cos(rotation.y - std::numbers::pi / 2.0f)
        ));
        const auto& up = cross(right, direction);

        return lookAt(
            position,
            position + direction,
            up
        );
    }

    /**
     * \brief Calculates the perspective matrix for a given aspect ratio and the current camera's settings.
     * \param aspectRatio The aspect ratio (width / height) in degrees.
     * \return Returns the perspective matrix for the given aspect ratio and camera settings.
     */
    glm::mat4 Camera::perspective(const float aspectRatio) const {
        return glm::perspective(
            glm::radians(fieldOfView),
            aspectRatio,
            nearPlane,
            farPlane
        );
    }

    const glm::vec3& Camera::getPosition() const {
        return position;
    }

    glm::vec3 Camera::getEulerRotation() const {
        return rotation;
    }

    float Camera::getNearPlane() const {
        return nearPlane;
    }

    float Camera::getFarPlane() const {
        return farPlane;
    }
}
