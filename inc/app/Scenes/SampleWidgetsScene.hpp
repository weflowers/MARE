#ifndef SAMPLEWIDGETSCENE
#define SAMPLEWIDGETSCENE

// MARE
#include "app/Entities/SampleEntity.hpp"
#include "app/Layers/SampleWidgetsLayer.hpp"
#include "mare/Entities/Billboard.hpp"
#include "mare/Renderer.hpp"
#include "mare/Scene.hpp"

#include "mare/Assets/Materials/PhongMaterial.hpp"
#include "mare/Assets/Meshes/CubeMesh.hpp"
#include "mare/Systems/Controls/OrbitControls.hpp"

namespace mare {

/**
 * @brief An example of a Scene with the SampleWidgetsLayer
 *
 */
class SampleWidgetsScene : public Scene {
public:
  SampleWidgetsScene() : Scene(ProjectionType::PERSPECTIVE) {

    // generate and push layers onto the stack
    gen_layer<SampleWidgetsLayer>();

    // set z as the up direction, and the initial position and direction of the
    // camera. This is required to use the OrbitConotrls System.
    set_position({0.0f, -1.0f, 0.0f});
    face_towards({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});

    // Extruded Text
    sample_text =
        gen_entity<Billboard>("Hello, World!", 1.0f / 17.0f, 3.0f / 17.0f, 182);
    sample_text->face_towards({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f});

    // floor
    floor = gen_ref<CubeMesh>(1.0f);
    floor->set_scale({1.0f, 1.0f, 0.01f});
    floor_material = gen_ref<PhongMaterial>();
    light = gen_ref<Spotlight>();
    floor_material->set_light(light);

    gen_system<OrbitControls>();
  }

  void on_enter() override {}

  void render(float delta_time) override {
    // Renderer properties
    Renderer::enable_blending(true);
    Renderer::enable_depth_testing(true);
    Renderer::enable_face_culling(true);
    // Clear color and depth buffer
    Renderer::clear_color_buffer(bg_color);
    Renderer::clear_depth_buffer();

    if (get_layer<SampleWidgetsLayer>()->get_entity<Switch>()->get_value()) {
      Renderer::wireframe_mode(true);
    } else {
      Renderer::wireframe_mode(false);
    }

    float scale =
        get_layer<SampleWidgetsLayer>()->get_entity<Slider>()->get_value();
    sample_text->set_scale(glm::vec3(scale));

    // t += delta_time / 10.0f;
    // float x = r * cos(t);
    // float y = r * sin(t);
    // float z = r * sin(t);
    // set_position(glm::vec3(x, y, z));
    // look_at(glm::vec3(0.0f));

    // render sample text
    glm::vec4 ambient_color =
        get_layer<SampleWidgetsLayer>()->get_entity<ColorPicker>()->get_value();
    sample_text->set_color(ambient_color);

    floor->render(this, floor_material.get());
  }

  void on_exit() override {}

private:
  glm::vec4 bg_color{0.12f, 0.12f, 0.12f, 1.0f};
  // testing
  Referenced<Billboard> sample_text;
  Referenced<CubeMesh> floor;
  Referenced<PhongMaterial> floor_material;
  Referenced<Spotlight> light;
  float t = 0.0f;
  float r = 2.0f;
};

} // namespace mare

#endif