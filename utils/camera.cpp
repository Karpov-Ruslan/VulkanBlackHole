#include "camera.hpp"

#include "utils/window.hpp"
#include <cmath>
#include <numbers>

namespace KRV {

Camera::Camera(glm::vec3 position, glm::vec3 direction, float speed, float rotation_speed, float fov) :
    position(position), direction(glm::normalize(direction)), speed(speed), rotation_speed(rotation_speed), fov(fov) {
    polarAngle = std::asin(this->direction.z);
    azimutalAngle = std::atan2(this->direction.y, this->direction.x);
}

void Camera::Update() {
    auto const &events = Window::GetInstance().GetEvents();

    float time = clock.Reset();

    constexpr static float PI_2 = std::numbers::pi/2.0F - 0.05F;
    if (events.keyboard.ARROW_UP) {polarAngle = std::clamp(polarAngle + rotation_speed*time, -PI_2, PI_2);}
    if (events.keyboard.ARROW_DOWN) {polarAngle = std::clamp(polarAngle - rotation_speed*time, -PI_2, PI_2);}
    if (events.keyboard.ARROW_LEFT) {azimutalAngle += rotation_speed*time;}
    if (events.keyboard.ARROW_RIGHT) {azimutalAngle -= rotation_speed*time;}
    direction = glm::vec3(std::cos(polarAngle)*std::cos(azimutalAngle),
        std::cos(polarAngle)*std::sin(azimutalAngle), std::sin(polarAngle));

    if (events.keyboard.W) {position += glm::normalize(glm::vec3(direction.x, direction.y, 0.0F))*speed*time;}
    if (events.keyboard.S) {position -= glm::normalize(glm::vec3(direction.x, direction.y, 0.0F))*speed*time;}
    if (events.keyboard.A) {position -= glm::normalize(glm::vec3(direction.y, -direction.x, 0.0F))*speed*time;}
    if (events.keyboard.D) {position += glm::normalize(glm::vec3(direction.y, -direction.x, 0.0F))*speed*time;}
    if (events.keyboard.E) {position += glm::vec3(0.0F, 0.0F, 1.0F)*speed*time;}
    if (events.keyboard.Q) {position -= glm::vec3(0.0F, 0.0F, 1.0F)*speed*time;}
}

glm::vec3 const & Camera::GetPosition() const {
    return position;
}

glm::vec3 const & Camera::GetDirection() const {
    return direction;
}


}
