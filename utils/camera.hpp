#pragma once

#include <glm/glm.hpp>

#include "utils/clock.hpp"

namespace KRV {

class Camera final {
public:
    Camera(glm::vec3 position, glm::vec3 direction, float speed, float rotation_speed, float fov);

    void Update();

    glm::vec3 const & GetPosition() const;
    glm::vec3 const & GetDirection() const;

private:
    Clock clock;

    glm::vec3 position = glm::vec3(0.0F);
    glm::vec3 direction = glm::normalize(glm::vec3(1.0F));

    float speed = 0.0F;
    float rotation_speed = 0.0F;
    float fov = 0.0F;

    float polarAngle = 0.0F;
    float azimutalAngle = 0.0F;
};

}
