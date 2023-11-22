#include "Camera.h"

namespace Vixen {
    Camera::Camera(const glm::vec3 position, const float fieldOfView, const float nearPlane, const float farPlane, const glm::vec3 clearColor)
            : position(position),
//              rotation(glm::quat{}),
              rotation({}),
              fieldOfView(fieldOfView),
              nearPlane(nearPlane),
              farPlane(farPlane),
              clearColor(clearColor) {}

    glm::mat4 Camera::view() const {
//        const auto &reverse = glm::conjugate(rotation);
//        glm::mat4 rot = glm::toMat4(reverse);
//        glm::mat4 translation = glm::translate({1.0}, -position);
//
//        return rot * translation;
        // return glm::lookAt(
        //         position,
        //         position + rotation,
        //         {0.0f, 1.0f, 0.0f}
        // );
        return glm::lookAt(
            glm::vec3{2.0f, 2.0f, 2.0f},
            glm::vec3{0.0f, 0.0f, 0.0f},
            glm::vec3{0.0f, 0.0f, 1.0f}
        );
    }

    glm::mat4 Camera::perspective(const float aspectRatio) const {
        // return glm::perspective(
        //         glm::radians(fieldOfView),
        //         aspectRatio,
        //         nearPlane,
        //         farPlane
        // );
        return glm::perspective(
            glm::radians(45.0f),
            1.7f,
            0.1f,
            10.0f
        );
    }

    const glm::vec3 &Camera::getPosition() const {
        return position;
    }

//    const glm::quat &Camera::getRotation() const {
//        return rotation;
//    }

    glm::vec3 Camera::getEulerRotation() const {
//        return glm::eulerAngles(rotation);
        return rotation;
    }

    float Camera::getNearPlane() const {
        return nearPlane;
    }

    float Camera::getFarPlane() const {
        return farPlane;
    }

    const glm::vec3 &Camera::getClearColor() const {
        return clearColor;
    }
}
