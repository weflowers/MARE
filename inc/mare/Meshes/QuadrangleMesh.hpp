#ifndef QUADRANGLEMESH
#define QUADRANGLEMESH

#include "mare/SimpleMesh.hpp"
#include "mare/Application.hpp"

#include "glm.hpp"

namespace mare
{
class QuadrangleMesh : public SimpleMesh<float>
{
public:
    QuadrangleMesh(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3, glm::vec2 v4)
    {
        std::vector<float> verts;
        render_state->set_draw_method(DrawMethod::TRIANGLES);

        verts.push_back(v1[0]);
        verts.push_back(v1[1]);
        verts.push_back(v2[0]);
        verts.push_back(v2[1]);
        verts.push_back(v3[0]);
        verts.push_back(v3[1]);
        verts.push_back(v4[0]);
        verts.push_back(v4[1]);

        vertex_buffers = Application::GenBuffer<float>(1);
        vertex_buffers->create(verts);
        vertex_buffers->set_format({{ShaderDataType::VEC2, "position"}});

        render_state->create();
        render_state->add_vertex_buffer(vertex_buffers);
    }
};
} // namespace mare

#endif