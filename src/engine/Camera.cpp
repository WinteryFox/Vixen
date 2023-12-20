#include "Camera.h"

namespace Vixen {
    Camera::Camera(
        const glm::vec3 position,
        const glm::vec3 rotation,
        const float fieldOfView,
        const float nearPlane,
        const float farPlane
    ) : position(position),
        rotation(rotation),
        fieldOfView(fieldOfView),
        nearPlane(nearPlane),
        farPlane(farPlane) {}

    /**
     * \brief Calculates the view matrix for this camera.
     * \return Returns the view matrix.
     */
    glm::mat4 Camera::view() const {
        const auto& direction = normalize(glm::vec3(
            cos(rotation.x) * sin(rotation.y),
            sin(rotation.y),
            cos(rotation.x) * cos(rotation.y)
        ));
        const auto& right = normalize(glm::vec3(
            sin(rotation.y - M_PI / 2.0f),
            0.0f,
            cos(rotation.y - M_PI / 2.0f)
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
