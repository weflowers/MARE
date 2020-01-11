#ifndef GLMESHFACTORY
#define GLMESHFACTORY

// MARE
#include "mare/MeshFactory.hpp"
// External Libraries
#include "glm.hpp"

namespace mare
{
    class GLMeshFactory : public MeshFactory
    {
        public:
        virtual ~GLMeshFactory(){}
        Mesh* GenTriangle(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3) override;
    };
}

#endif