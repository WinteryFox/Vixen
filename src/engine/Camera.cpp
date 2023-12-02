#include "Camera.h"

namespace Vixen {
    Camera::Camera(const glm::vec3 position, const float fieldOfView, const float nearPlane, const float farPlane,
                   const glm::vec3 clearColor)
        : position(position),
          //              rotation(glm::quat{}),
          rotation({}),
          fieldOfView(fieldOfView),
          nearPlane(nearPlane),
          farPlane(farPlane),
          clearColor(clearColor) {}

    glm::mat4 Camera::view() const {
        // glm::vec3 front;
        // front.x = static_cast<float>(cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y)));
        // front.y = static_cast<float>(sin(glm::radians(rotation.y)));
        // front.z = static_cast<float>(sin(glm::radians(rotation.x)) * cos(glm::radians(rotation.y)));
        //
        // front = normalize(front);
        // const auto &right = normalize(cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        // const auto &up = normalize(cross(right, front));
        //
        // return lookAt(position, position + front, up);
        return lookAt(
            position,
            glm::vec3(0, 0, 0),
            {0.0f, 1.0f, 0.0f}
        );
    }

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

    const glm::vec3& Camera::getClearColor() const {
        return clearColor;
    }
}
