#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Vixen {
    class Camera {
        explicit Camera(
                float fieldOfView = 114.0f,
                float nearPlane = 0.01f,
                float farPlane = 1000.0f,
                glm::vec3 clearColor = {0.0f, 0.0f, 0.0f}
        );

        [[nodiscard]] glm::mat4 view() const;

        [[nodiscard]] glm::mat4 perspective(float aspectRatio) const;

    public:
        [[nodiscard]] const glm::vec3 &getPosition() const;

//        [[nodiscard]] const glm::quat &getRotation() const;

        [[nodiscard]] glm::vec3 getEulerRotation() const;

        [[nodiscard]] float getNearPlane() const;

        [[nodiscard]] float getFarPlane() const;

        [[nodiscard]] const glm::vec3 &getClearColor() const;

    private:
        glm::vec3 position;

        //glm::quat rotation;
        glm::vec3 rotation;

        float fieldOfView;

        float nearPlane;

        float farPlane;

        glm::vec3 clearColor;
    };
}
