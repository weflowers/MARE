#ifndef FLYCONTROLS
#define FLYCONTROLS

// MARE
#include "mare/MareUtility.hpp"
#include "mare/Systems.hpp"
#include "mare/Components/Physics/Rigidbody.hpp"

namespace mare
{

class FlyControls : public IControlsSystem
{
    float speed = 1.0f;
    bool on_key(Entity *entity, const RendererInput &input) override
    {
        if (auto rigidbody = entity->get_component<Rigidbody>())
        {
            if (input.LEFT_SHIFT_PRESSED)
            {
                speed = 2.5f;
            }
            else
            {
                speed = 1.0f;
            }
            float x = 0.0f;
            float y = 0.0f;
            if (input.W_PRESSED)
            {
                y += 1.0f;
            }
            if (input.S_PRESSED)
            {
                y -= 1.0f;
            }
            if (input.D_PRESSED)
            {
                x += 1.0f;
            }
            if (input.A_PRESSED)
            {
                x -= 1.0f;
            }
            if (x || y)
            {
                x /= sqrtf(x * x + y * y);
                y /= sqrtf(x * x + y * y);
            }
            glm::vec3 dir = entity->get_direction();
            glm::vec3 up = entity->get_up_vector();
            glm::vec3 right = glm::normalize(glm::cross(dir, up));
            glm::vec3 velocity = (dir * y + right * x) * speed;
            rigidbody->linear_velocity = velocity;
        }
        return false;
    }
    bool on_mouse_move(Entity *entity, const RendererInput &input) override
    {
        float sensitivity = 300.0f;
        float dtheta = float(input.mouse_vel.y) / sensitivity;
        float dphi = -float(input.mouse_vel.x) / sensitivity;
        glm::vec3 dir = entity->get_direction();

        float theta = acosf(dir.z);
        float phi = atan2f(dir.y, dir.x);

        theta = glm::clamp(theta + dtheta, 0.01f, PI - 0.01f);
        phi += dphi;

        dir = glm::vec3(sinf(theta) * cosf(phi), sinf(theta) * sinf(phi), cosf(theta));
        glm::vec3 right = glm::normalize(glm::cross(dir, {0.0f, 0.0f, 1.0f}));
        glm::vec3 up = glm::cross(right, dir);

        entity->face_towards(dir);
        entity->set_up_vector(up);
        return false;
    }
    bool on_mouse_button(Entity *entity, const RendererInput &input) override { return false; }
    bool on_mouse_wheel(Entity *entity, const RendererInput &input) override { return false; }
    bool on_resize(Entity *entity, const RendererInput &input) override { return false; }
};
} // namespace mare

#endif