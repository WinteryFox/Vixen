#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Vixen {
    // TODO: Ideally, the camera should be quaternion based and not euler-angles based.
    class Camera {
    public:
        explicit Camera(
            glm::vec3 position = {},
            glm::vec3 rotation = {M_PI, 0.0f, 0.0f},
            float fieldOfView = 90.0f,
            float nearPlane = 0.1f,
            float farPlane = 1000.0f
        );

        void update(GLFWwindow* window, double deltaTime);

        [[nodiscard]] glm::mat4 view() const;

        [[nodiscard]] glm::mat4 perspective(float aspectRatio) const;

        [[nodiscard]] const glm::vec3& getPosition() const;

        [[nodiscard]] glm::vec3 getEulerRotation() const;

        [[nodiscard]] float getNearPlane() const;

        [[nodiscard]] float getFarPlane() const;

    private:
        double lastX;

        double lastY;

        glm::vec3 position;

        glm::vec3 rotation;

        float fieldOfView;

        float nearPlane;

        float farPlane;
    };
}
