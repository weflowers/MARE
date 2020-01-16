#ifndef GLCOMPOSITEMESH
#define GLCOMPOSITEMESH

// Standard Library
#include <vector>
// MARE
#include "mare/GL/GLMesh.hpp"
#include "mare/CompositeMesh.hpp"

namespace mare
{
class GLCompositeMesh : public GLMesh, public CompositeMesh
{
public:
    GLCompositeMesh() {}
    virtual ~GLCompositeMesh() {}
    void render(Material *material) override;
    void render(Material *material, glm::mat4 model) override;
    void render(Material *material, glm::mat4 parent_model, unsigned int instance_count, Buffer<glm::mat4>* models) override;
};
} // namespace mare

#endif