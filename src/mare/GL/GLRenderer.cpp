// MARE GL
#include "mare/GL/GLRenderer.hpp"
#include "mare/GL/GLBuffer.hpp"
#include "mare/GL/GLShader.hpp"
#include "mare/GL/GLTexture.hpp"
#include "mare/GL/GLRenderState.hpp"

// MARE
#include "mare/SimpleMesh.hpp"
#include "mare/Scene.hpp"
#include "mare/Layer.hpp"
#include "mare/Widget.hpp"
#include "mare/Entity.hpp"

// Standard Library
#include <iostream>

namespace mare
{

GLenum GLDrawMethod(DrawMethod draw_method)
{
    switch (draw_method)
    {
    case DrawMethod::POINTS:
        return GL_POINTS;
    case DrawMethod::LINES:
        return GL_LINES;
    case DrawMethod::LINE_STRIP:
        return GL_LINE_STRIP;
    case DrawMethod::LINE_LOOP:
        return GL_LINE_LOOP;
    case DrawMethod::TRIANGLES:
        return GL_TRIANGLES;
    case DrawMethod::TRIANGLE_STRIP:
        return GL_TRIANGLE_STRIP;
    case DrawMethod::TRIANGLE_FAN:
        return GL_TRIANGLE_FAN;
    default:
        return GL_POINTS;
    }
}

void GLRenderer::init()
{
    running = true;

    if (!glfwInit())
    {
        std::cerr << "GLFW Failed to initialize." << std::endl;
        running = false;
        return;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    if (info.debug_mode.any())
    {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    }
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, info.samples);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    window = glfwCreateWindow(info.window_width, info.window_height, "Untitled", info.fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to open window." << std::endl;
        running = false;
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, glfw_onResize);
    glfwSetKeyCallback(window, glfw_onKey);
    glfwSetMouseButtonCallback(window, glfw_onMouseButton);
    glfwSetCursorPosCallback(window, glfw_onMouseMove);
    glfwSetScrollCallback(window, glfw_onMouseWheel);
    if (!info.cursor)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    hz_resize_cursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    arrow_cursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    hand_cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    crosshair_cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    if (info.vsync)
    {
        glfwSwapInterval(1);
    }
    else
    {
        glfwSwapInterval(0);
    }

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW failed to initialize." << std::endl;
    }

    if (info.debug_mode.any())
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debug_message_callback, this);
        std::cout << "GL VENDOR: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "GL VERSION: " << glGetString(GL_VERSION) << "\n";
        std::cout << "GL RENDERER: " << glGetString(GL_RENDERER) << std::endl;
    }
}

void GLRenderer::start_process()
{
    set_window_title(info.window_title);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    input.mouse_pos = glm::ivec2(xpos, ypos);
    info.current_time = glfwGetTime();
    do
    {
        double time = glfwGetTime();
        float delta_time = (float)(time - info.current_time);
        // Update render and physics components
        if (info.scene)
        {
            // Scene and camera components
            auto physics_systems = info.scene->get_components<IPhysicsSystem>();
            auto render_systems = info.scene->get_components<IRenderSystem>();
            for (auto system : physics_systems)
            {
                system->update(info.scene, delta_time);
            }
            for (auto system : render_systems)
            {
                system->render(info.scene, info.scene, delta_time);
            }
            info.scene->render(delta_time);
            // Entities in scene
            for (auto entity_it = info.scene->entity_begin(); entity_it != info.scene->entity_end(); entity_it++)
            {
                Entity *entity = entity_it->get();
                physics_systems = entity->get_components<IPhysicsSystem>();
                render_systems = entity->get_components<IRenderSystem>();
                for (auto system : physics_systems)
                {
                    system->update(entity, delta_time);
                }
                for (auto system : render_systems)
                {
                    system->render(entity, info.scene, delta_time);
                }
            }
            // Overlays on scene and widgets in overlays
            for (auto overlay_it = info.scene->overlay_begin(); overlay_it != info.scene->overlay_end(); overlay_it++)
            {
                Overlay *overlay = overlay_it->get();
                physics_systems = overlay->get_components<IPhysicsSystem>();
                render_systems = overlay->get_components<IRenderSystem>();
                for (auto system : physics_systems)
                {
                    system->update(overlay, delta_time);
                }
                for (auto system : render_systems)
                {
                    system->render(overlay, overlay, delta_time);
                }
                overlay->render(delta_time);
                for (auto widget_it = overlay->widget_begin(); widget_it != overlay->widget_end(); widget_it++)
                {
                    Widget *widget = widget_it->get();
                    physics_systems = widget->get_components<IPhysicsSystem>();
                    render_systems = widget->get_components<IRenderSystem>();
                    for (auto system : physics_systems)
                    {
                        system->update(widget, delta_time);
                    }
                    for (auto system : render_systems)
                    {
                        system->render(widget, overlay, delta_time);
                    }
                }
            }
        }
        info.current_time = time;
        glfwPollEvents();
        glfwSwapBuffers(window);
    } while (running && !glfwWindowShouldClose(window));
    glfwDestroyCursor(hz_resize_cursor);
    glfwDestroyCursor(arrow_cursor);
    glfwDestroyCursor(hand_cursor);
    glfwDestroyCursor(crosshair_cursor);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GLRenderer::set_window_title(const char *title)
{
    glfwSetWindowTitle(window, title);
}

void GLRenderer::set_cursor(CURSOR type)
{
    switch (type)
    {
    case CURSOR::ARROW:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursor(window, arrow_cursor);
        break;
    case CURSOR::HZ_RESIZE:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursor(window, hz_resize_cursor);
        break;
    case CURSOR::HAND:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursor(window, hand_cursor);
        break;
    case CURSOR::CROSSHAIRS:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursor(window, crosshair_cursor);
        break;
    case CURSOR::DISABLED:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    case CURSOR::ENABLED:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    }
}

void GLRenderer::clear_color_buffer(glm::vec4 color)
{
    glClearBufferfv(GL_COLOR, 0, &color[0]);
}

void GLRenderer::clear_depth_buffer()
{
    glClear(GL_DEPTH_BUFFER_BIT);
}

void GLRenderer::resize_window(int width, int height)
{
    info.window_width = width;
    info.window_height = height;
    info.window_aspect = float(info.window_width) / float(info.window_height);
    glViewport(0, 0, width, height);
}

void GLRenderer::wireframe_mode(bool wireframe)
{
    if (wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void GLRenderer::enable_depth_testing(bool enable)
{
    if (enable)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }
}

void GLRenderer::enable_face_culling(bool enable)
{
    if (enable)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void GLRenderer::enable_blending(bool enable)
{
    if (enable)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

glm::vec3 GLRenderer::raycast(Layer *layer)
{
    glm::mat4 inversed_camera = glm::inverse(layer->projection() * layer->view());
    float x = 2.0f * (float)input.mouse_pos.x / (float)(info.window_width) - 1.0f;
    float y = -2.0f * (float)input.mouse_pos.y / (float)(info.window_height) + 1.0f;
    float z = 0.0f;
    glReadPixels(input.mouse_pos.x, info.window_height - input.mouse_pos.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
    z = 2.0f * z - 1.0f;
    glm::vec4 screen_vector = glm::vec4(x, y, z, 1.0f);
    glm::vec4 world_vector = inversed_camera * screen_vector;
    world_vector /= world_vector.w;
    return glm::vec3(world_vector);
}

glm::vec3 GLRenderer::raycast(Layer *layer, glm::ivec2 screen_coords)
{
    glm::mat4 inversed_camera = glm::inverse(layer->projection() * layer->view());
    float x = 2.0f * (float)screen_coords.x / (float)(info.window_width) - 1.0f;
    float y = -2.0f * (float)screen_coords.y / (float)(info.window_height) + 1.0f;
    float z = 0.0f;
    glReadPixels(screen_coords.x, info.window_height - screen_coords.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
    z = 2.0f * z - 1.0f;
    glm::vec4 screen_vector = glm::vec4(x, y, z, 1.0f);
    glm::vec4 world_vector = inversed_camera * screen_vector;
    world_vector /= world_vector.w;
    return glm::vec3(world_vector);
}

// Buffers
Scoped<Buffer<float>> GLRenderer::GenFloatBuffer(std::vector<float> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<float>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<int>> GLRenderer::GenIntBuffer(std::vector<int> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<int>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<unsigned int>> GLRenderer::GenIndexBuffer(std::vector<unsigned int> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<unsigned int>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<bool>> GLRenderer::GenBoolBuffer(std::vector<bool> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<bool>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<glm::vec2>> GLRenderer::GenVec2Buffer(std::vector<glm::vec2> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<glm::vec2>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<glm::vec3>> GLRenderer::GenVec3Buffer(std::vector<glm::vec3> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<glm::vec3>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<glm::vec4>> GLRenderer::GenVec4Buffer(std::vector<glm::vec4> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<glm::vec4>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<glm::mat2>> GLRenderer::GenMat2Buffer(std::vector<glm::mat2> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<glm::mat2>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<glm::mat3>> GLRenderer::GenMat3Buffer(std::vector<glm::mat3> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<glm::mat3>>(data, buffer_type, size_in_bytes);
}
Scoped<Buffer<glm::mat4>> GLRenderer::GenMat4Buffer(std::vector<glm::mat4> *data, BufferType buffer_type, size_t size_in_bytes)
{
    return std::make_unique<GLBuffer<glm::mat4>>(data, buffer_type, size_in_bytes);
}

// Textures
Scoped<Texture2D> GLRenderer::GenTexture2D(const char *image_filepath)
{
    return std::make_unique<GLTexture2D>(image_filepath);
}

// Render States
Scoped<RenderState> GLRenderer::GenRenderState()
{
    return std::make_unique<GLRenderState>();
}

// Shaders
Scoped<Shader> GLRenderer::GenShader(const char *directory)
{
    return std::make_unique<GLShader>(directory);
}

// normal rendering of simple meshes
void GLRenderer::render_simple_mesh(const Layer *layer, const SimpleMesh *mesh, Material *material)
{
    mesh->bind();
    material->bind();
    if (layer)
    {
        material->upload_mat4("projection", layer->projection());
        material->upload_mat4("view", layer->view());
    }
    material->upload_mat4("model", mesh->get_model());
    material->upload_mat3("normal_matrix", glm::mat3(mesh->get_normal()));
    if (mesh->get_state()->is_indexed())
    {
        glDrawElementsBaseVertex(GLDrawMethod(mesh->get_draw_method()), GLsizei(mesh->get_state()->render_count()), GL_UNSIGNED_INT, nullptr, mesh->get_state()->get_render_index());
    }
    else
    {
        glDrawArrays(GLDrawMethod(mesh->get_draw_method()), mesh->get_state()->get_render_index(), GLsizei(mesh->get_state()->render_count()));
    }
    mesh->get_state()->lock_buffer();
    mesh->get_state()->swap_buffer();
}
// composite rendering
void GLRenderer::render_simple_mesh(const Layer *layer, const SimpleMesh *mesh, Material *material, const glm::mat4 &parent_model)
{
    mesh->bind();
    material->bind();
    if (layer)
    {
        material->upload_mat4("projection", layer->projection());
        material->upload_mat4("view", layer->view());
    }
    material->upload_mat4("model", parent_model * mesh->get_model());
    material->upload_mat3("normal_matrix", glm::transpose(glm::inverse(glm::mat3(parent_model * (mesh->get_model())))));
    if (mesh->get_state()->is_indexed())
    {
        glDrawElementsBaseVertex(GLDrawMethod(mesh->get_draw_method()), GLsizei(mesh->get_state()->render_count()), GL_UNSIGNED_INT, nullptr, mesh->get_state()->get_render_index());
    }
    else
    {
        glDrawArrays(GLDrawMethod(mesh->get_draw_method()), mesh->get_state()->get_render_index(), GLsizei(mesh->get_state()->render_count()));
    }
    mesh->get_state()->lock_buffer();
    mesh->get_state()->swap_buffer();
}
// instanced rendering
void GLRenderer::render_simple_mesh(const Layer *layer, const SimpleMesh *mesh, Material *material, const glm::mat4 &parent_model, unsigned int instance_count, const Buffer<glm::mat4> *models)
{
    mesh->bind();
    material->bind();
    if (layer)
    {
        material->upload_mat4("projection", layer->projection());
        material->upload_mat4("view", layer->view());
    }
    material->upload_mat4("model", parent_model * mesh->get_model());
    material->upload_mat3("normal_matrix", glm::transpose(glm::inverse(glm::mat3(parent_model * (mesh->get_model())))));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, models->name());
    if (mesh->get_state()->is_indexed())
    {
        glDrawElementsInstancedBaseVertex(GLDrawMethod(mesh->get_draw_method()), static_cast<GLsizei>(mesh->get_state()->render_count()), GL_UNSIGNED_INT, nullptr, instance_count, mesh->get_state()->get_render_index());
    }
    else
    {
        glDrawArraysInstanced(GLDrawMethod(mesh->get_draw_method()), mesh->get_state()->get_render_index(), static_cast<GLsizei>(mesh->get_state()->render_count()), instance_count);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    mesh->get_state()->lock_buffer();
    mesh->get_state()->swap_buffer();
}

// Debug functions
std::string GLRenderer::debug_source_string(GLenum source)
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "APPLICATION";
    case GL_DEBUG_SOURCE_OTHER:
        return "OTHER";
    }
    return "INVALID_SOURCE";
}
std::string GLRenderer::debug_type_string(GLenum type)
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRICATED BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UNDEFINED BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERFORMANCE";
    case GL_DEBUG_TYPE_OTHER:
        return "OTHER";
    case GL_DEBUG_TYPE_MARKER:
        return "MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "PUSH GROUP";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "POP GROUP";
    }
    return "INVALID_TYPE";
}
std::string GLRenderer::debug_severity_string(GLenum severity)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        return "***HIGH***";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "**MEDIUM**";
    case GL_DEBUG_SEVERITY_LOW:
        return "*LOW*";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "NOTIFICATION";
    }
    return "INVALID_SEVERITY";
}
void GLAPIENTRY
GLRenderer::debug_message_callback(GLenum source,
                                   GLenum type,
                                   GLuint id,
                                   GLenum severity,
                                   GLsizei length,
                                   const GLchar *message,
                                   const void *userParam)
{
    auto debug_state = info.debug_mode;
    if (debug_state[0] && severity == GL_DEBUG_SEVERITY_HIGH)
    {
        std::cerr << "GL_" << debug_source_string(source) << "_" << debug_type_string(type) << ": "
                  << debug_severity_string(severity) << " - " << message << std::endl;
    }
    if (debug_state[1] && severity == GL_DEBUG_SEVERITY_MEDIUM)
    {
        std::cerr << "GL_" << debug_source_string(source) << "_" << debug_type_string(type) << ": "
                  << debug_severity_string(severity) << " - " << message << std::endl;
    }
    if (debug_state[2] && severity == GL_DEBUG_SEVERITY_LOW)
    {
        std::cerr << "GL_" << debug_source_string(source) << "_" << debug_type_string(type) << ": "
                  << debug_severity_string(severity) << " - " << message << std::endl;
    }
    if (debug_state[3] && severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        std::cerr << "GL_" << debug_source_string(source) << "_" << debug_type_string(type) << ": "
                  << debug_severity_string(severity) << " - " << message << std::endl;
    }
}

// Renderer callback functions
void GLRenderer::glfw_onResize(GLFWwindow *window, int w, int h)
{
    // callback to the renderer to resize the viewport
    Renderer::API->resize_window(w, h);

    if (info.scene)
    {
        // reverse iterate through overlay callbacks first
        bool handled = false;
        for (auto overlay_it = info.scene->overlay_rbegin(); overlay_it != info.scene->overlay_rend(); overlay_it++)
        {
            Overlay *overlay = overlay_it->get();
            // reverse iterate through overlay widgets
            for (auto widget_it = overlay->widget_rbegin(); widget_it != overlay->widget_rend(); widget_it++)
            {
                Widget *widget = widget_it->get();
                auto controls_systems = widget->get_components<IControlsSystem>();
                // reverse iterate through widget controls callbacks
                for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
                {
                    handled = (*controls_it)->on_resize(widget, input);
                    if (handled)
                    {
                        return;
                    }
                }
            }
            auto controls_systems = overlay->get_components<IControlsSystem>();
            // reverse iterate through overlay controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_resize(overlay, input);
                if (handled)
                {
                    return;
                }
            }
        }
        // if event is not handled by overlays, callback to the scene
        // reverse iterate through scene entities
        for (auto entity_it = info.scene->entity_rbegin(); entity_it != info.scene->entity_rend(); entity_it++)
        {
            Entity *entity = entity_it->get();
            auto controls_systems = entity->get_components<IControlsSystem>();
            // reverse iterate through entity controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_resize(entity, input);
                if (handled)
                {
                    return;
                }
            }
        }
        auto controls_systems = info.scene->get_components<IControlsSystem>();
        // reverse iterate through scene controls callbacks
        for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
        {
            handled = (*controls_it)->on_resize(info.scene, input);
            if (handled)
            {
                return;
            }
        }
    }
}

void GLRenderer::glfw_onKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS)
        {
            input.SPACE_PRESSED = true;
            input.SPACE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.SPACE_PRESSED = true;
            input.SPACE_JUST_PRESSED = false;
        }
        else
        {
            input.SPACE_PRESSED = false;
            input.SPACE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_APOSTROPHE:
        if (action == GLFW_PRESS)
        {
            input.APOSTROPHE_PRESSED = true;
            input.APOSTROPHE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.APOSTROPHE_PRESSED = true;
            input.APOSTROPHE_JUST_PRESSED = false;
        }
        else
        {
            input.APOSTROPHE_PRESSED = false;
            input.APOSTROPHE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_COMMA:
        if (action == GLFW_PRESS)
        {
            input.COMMA_PRESSED = true;
            input.COMMA_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.COMMA_PRESSED = true;
            input.COMMA_JUST_PRESSED = false;
        }
        else
        {
            input.COMMA_PRESSED = false;
            input.COMMA_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_MINUS:
        if (action == GLFW_PRESS)
        {
            input.MINUS_PRESSED = true;
            input.MINUS_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.MINUS_PRESSED = true;
            input.MINUS_JUST_PRESSED = false;
        }
        else
        {
            input.MINUS_PRESSED = false;
            input.MINUS_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_PERIOD:
        if (action == GLFW_PRESS)
        {
            input.PERIOD_PRESSED = true;
            input.PERIOD_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.PERIOD_PRESSED = true;
            input.PERIOD_JUST_PRESSED = false;
        }
        else
        {
            input.PERIOD_PRESSED = false;
            input.PERIOD_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_SLASH:
        if (action == GLFW_PRESS)
        {
            input.FORWARD_SLASH_PRESSED = true;
            input.FORWARD_SLASH_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.FORWARD_SLASH_PRESSED = true;
            input.FORWARD_SLASH_JUST_PRESSED = false;
        }
        else
        {
            input.FORWARD_SLASH_PRESSED = false;
            input.FORWARD_SLASH_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_0:
        if (action == GLFW_PRESS)
        {
            input.ZERO_PRESSED = true;
            input.ZERO_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.ZERO_PRESSED = true;
            input.ZERO_JUST_PRESSED = false;
        }
        else
        {
            input.ZERO_PRESSED = false;
            input.ZERO_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_1:
        if (action == GLFW_PRESS)
        {
            input.ONE_PRESSED = true;
            input.ONE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.ONE_PRESSED = true;
            input.ONE_JUST_PRESSED = false;
        }
        else
        {
            input.ONE_PRESSED = false;
            input.ONE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_2:
        if (action == GLFW_PRESS)
        {
            input.TWO_PRESSED = true;
            input.TWO_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.TWO_PRESSED = true;
            input.TWO_JUST_PRESSED = false;
        }
        else
        {
            input.TWO_PRESSED = false;
            input.TWO_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_3:
        if (action == GLFW_PRESS)
        {
            input.THREE_PRESSED = true;
            input.THREE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.THREE_PRESSED = true;
            input.THREE_JUST_PRESSED = false;
        }
        else
        {
            input.THREE_PRESSED = false;
            input.THREE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_4:
        if (action == GLFW_PRESS)
        {
            input.FOUR_PRESSED = true;
            input.FOUR_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.FOUR_PRESSED = true;
            input.FOUR_JUST_PRESSED = false;
        }
        else
        {
            input.FOUR_PRESSED = false;
            input.FOUR_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_5:
        if (action == GLFW_PRESS)
        {
            input.FIVE_PRESSED = true;
            input.FIVE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.FIVE_PRESSED = true;
            input.FIVE_JUST_PRESSED = false;
        }
        else
        {
            input.FIVE_PRESSED = false;
            input.FIVE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_6:
        if (action == GLFW_PRESS)
        {
            input.SIX_PRESSED = true;
            input.SIX_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.SIX_PRESSED = true;
            input.SIX_JUST_PRESSED = false;
        }
        else
        {
            input.SIX_PRESSED = false;
            input.SIX_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_7:
        if (action == GLFW_PRESS)
        {
            input.SEVEN_PRESSED = true;
            input.SEVEN_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.SEVEN_PRESSED = true;
            input.SEVEN_JUST_PRESSED = false;
        }
        else
        {
            input.SEVEN_PRESSED = false;
            input.SEVEN_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_8:
        if (action == GLFW_PRESS)
        {
            input.EIGHT_PRESSED = true;
            input.EIGHT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.EIGHT_PRESSED = true;
            input.EIGHT_JUST_PRESSED = false;
        }
        else
        {
            input.EIGHT_PRESSED = false;
            input.EIGHT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_9:
        if (action == GLFW_PRESS)
        {
            input.NINE_PRESSED = true;
            input.NINE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.NINE_PRESSED = true;
            input.NINE_JUST_PRESSED = false;
        }
        else
        {
            input.NINE_PRESSED = false;
            input.NINE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_SEMICOLON:
        if (action == GLFW_PRESS)
        {
            input.SEMICOLON_PRESSED = true;
            input.SEMICOLON_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.SEMICOLON_PRESSED = true;
            input.SEMICOLON_JUST_PRESSED = false;
        }
        else
        {
            input.SEMICOLON_PRESSED = false;
            input.SEMICOLON_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_EQUAL:
        if (action == GLFW_PRESS)
        {
            input.EQUAL_PRESSED = true;
            input.EQUAL_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.EQUAL_PRESSED = true;
            input.EQUAL_JUST_PRESSED = false;
        }
        else
        {
            input.EQUAL_PRESSED = false;
            input.EQUAL_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_A:
        if (action == GLFW_PRESS)
        {
            input.A_PRESSED = true;
            input.A_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.A_PRESSED = true;
            input.A_JUST_PRESSED = false;
        }
        else
        {
            input.A_PRESSED = false;
            input.A_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_B:
        if (action == GLFW_PRESS)
        {
            input.B_PRESSED = true;
            input.B_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.B_PRESSED = true;
            input.B_JUST_PRESSED = false;
        }
        else
        {
            input.B_PRESSED = false;
            input.B_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_C:
        if (action == GLFW_PRESS)
        {
            input.C_PRESSED = true;
            input.C_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.C_PRESSED = true;
            input.C_JUST_PRESSED = false;
        }
        else
        {
            input.C_PRESSED = false;
            input.C_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_D:
        if (action == GLFW_PRESS)
        {
            input.D_PRESSED = true;
            input.D_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.D_PRESSED = true;
            input.D_JUST_PRESSED = false;
        }
        else
        {
            input.D_PRESSED = false;
            input.D_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_E:
        if (action == GLFW_PRESS)
        {
            input.E_PRESSED = true;
            input.E_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.E_PRESSED = true;
            input.E_JUST_PRESSED = false;
        }
        else
        {
            input.E_PRESSED = false;
            input.E_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F:
        if (action == GLFW_PRESS)
        {
            input.F_PRESSED = true;
            input.F_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F_PRESSED = true;
            input.F_JUST_PRESSED = false;
        }
        else
        {
            input.F_PRESSED = false;
            input.F_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_G:
        if (action == GLFW_PRESS)
        {
            input.G_PRESSED = true;
            input.G_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.G_PRESSED = true;
            input.G_JUST_PRESSED = false;
        }
        else
        {
            input.G_PRESSED = false;
            input.G_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_H:
        if (action == GLFW_PRESS)
        {
            input.H_PRESSED = true;
            input.H_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.H_PRESSED = true;
            input.H_JUST_PRESSED = false;
        }
        else
        {
            input.H_PRESSED = false;
            input.H_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_I:
        if (action == GLFW_PRESS)
        {
            input.I_PRESSED = true;
            input.I_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.I_PRESSED = true;
            input.I_JUST_PRESSED = false;
        }
        else
        {
            input.I_PRESSED = false;
            input.I_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_J:
        if (action == GLFW_PRESS)
        {
            input.J_PRESSED = true;
            input.J_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.J_PRESSED = true;
            input.J_JUST_PRESSED = false;
        }
        else
        {
            input.J_PRESSED = false;
            input.J_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_K:
        if (action == GLFW_PRESS)
        {
            input.K_PRESSED = true;
            input.K_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.K_PRESSED = true;
            input.K_JUST_PRESSED = false;
        }
        else
        {
            input.K_PRESSED = false;
            input.K_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_L:
        if (action == GLFW_PRESS)
        {
            input.L_PRESSED = true;
            input.L_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.L_PRESSED = true;
            input.L_JUST_PRESSED = false;
        }
        else
        {
            input.L_PRESSED = false;
            input.L_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_M:
        if (action == GLFW_PRESS)
        {
            input.M_PRESSED = true;
            input.M_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.M_PRESSED = true;
            input.M_JUST_PRESSED = false;
        }
        else
        {
            input.M_PRESSED = false;
            input.M_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_N:
        if (action == GLFW_PRESS)
        {
            input.N_PRESSED = true;
            input.N_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.N_PRESSED = true;
            input.N_JUST_PRESSED = false;
        }
        else
        {
            input.N_PRESSED = false;
            input.N_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_O:
        if (action == GLFW_PRESS)
        {
            input.O_PRESSED = true;
            input.O_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.O_PRESSED = true;
            input.O_JUST_PRESSED = false;
        }
        else
        {
            input.O_PRESSED = false;
            input.O_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_P:
        if (action == GLFW_PRESS)
        {
            input.P_PRESSED = true;
            input.P_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.P_PRESSED = true;
            input.P_JUST_PRESSED = false;
        }
        else
        {
            input.P_PRESSED = false;
            input.P_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_Q:
        if (action == GLFW_PRESS)
        {
            input.Q_PRESSED = true;
            input.Q_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.Q_PRESSED = true;
            input.Q_JUST_PRESSED = false;
        }
        else
        {
            input.Q_PRESSED = false;
            input.Q_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_R:
        if (action == GLFW_PRESS)
        {
            input.R_PRESSED = true;
            input.R_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.R_PRESSED = true;
            input.R_JUST_PRESSED = false;
        }
        else
        {
            input.R_PRESSED = false;
            input.R_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_S:
        if (action == GLFW_PRESS)
        {
            input.S_PRESSED = true;
            input.S_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.S_PRESSED = true;
            input.S_JUST_PRESSED = false;
        }
        else
        {
            input.S_PRESSED = false;
            input.S_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_T:
        if (action == GLFW_PRESS)
        {
            input.T_PRESSED = true;
            input.T_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.T_PRESSED = true;
            input.T_JUST_PRESSED = false;
        }
        else
        {
            input.T_PRESSED = false;
            input.T_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_U:
        if (action == GLFW_PRESS)
        {
            input.U_PRESSED = true;
            input.U_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.U_PRESSED = true;
            input.U_JUST_PRESSED = false;
        }
        else
        {
            input.U_PRESSED = false;
            input.U_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_V:
        if (action == GLFW_PRESS)
        {
            input.V_PRESSED = true;
            input.V_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.V_PRESSED = true;
            input.V_JUST_PRESSED = false;
        }
        else
        {
            input.V_PRESSED = false;
            input.V_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_W:
        if (action == GLFW_PRESS)
        {
            input.W_PRESSED = true;
            input.W_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.W_PRESSED = true;
            input.W_JUST_PRESSED = false;
        }
        else
        {
            input.W_PRESSED = false;
            input.W_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_X:
        if (action == GLFW_PRESS)
        {
            input.X_PRESSED = true;
            input.X_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.X_PRESSED = true;
            input.X_JUST_PRESSED = false;
        }
        else
        {
            input.X_PRESSED = false;
            input.X_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_Y:
        if (action == GLFW_PRESS)
        {
            input.Y_PRESSED = true;
            input.Y_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.Y_PRESSED = true;
            input.Y_JUST_PRESSED = false;
        }
        else
        {
            input.Y_PRESSED = false;
            input.Y_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_Z:
        if (action == GLFW_PRESS)
        {
            input.Z_PRESSED = true;
            input.Z_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.Z_PRESSED = true;
            input.Z_JUST_PRESSED = false;
        }
        else
        {
            input.Z_PRESSED = false;
            input.Z_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_LEFT_BRACKET:
        if (action == GLFW_PRESS)
        {
            input.LEFT_BRACKET_PRESSED = true;
            input.LEFT_BRACKET_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.LEFT_BRACKET_PRESSED = true;
            input.LEFT_BRACKET_JUST_PRESSED = false;
        }
        else
        {
            input.LEFT_BRACKET_PRESSED = false;
            input.LEFT_BRACKET_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_RIGHT_BRACKET:
        if (action == GLFW_PRESS)
        {
            input.RIGHT_BRACKET_PRESSED = true;
            input.RIGHT_BRACKET_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.RIGHT_BRACKET_PRESSED = true;
            input.RIGHT_BRACKET_JUST_PRESSED = false;
        }
        else
        {
            input.RIGHT_BRACKET_PRESSED = false;
            input.RIGHT_BRACKET_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_GRAVE_ACCENT:
        if (action == GLFW_PRESS)
        {
            input.GRAVE_ACCENT_PRESSED = true;
            input.GRAVE_ACCENT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.GRAVE_ACCENT_PRESSED = true;
            input.GRAVE_ACCENT_JUST_PRESSED = false;
        }
        else
        {
            input.GRAVE_ACCENT_PRESSED = false;
            input.GRAVE_ACCENT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS)
        {
            input.ESCAPE_PRESSED = true;
            input.ESCAPE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.ESCAPE_PRESSED = true;
            input.ESCAPE_JUST_PRESSED = false;
        }
        else
        {
            input.ESCAPE_PRESSED = false;
            input.ESCAPE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_ENTER:
        if (action == GLFW_PRESS)
        {
            input.ENTER_PRESSED = true;
            input.ENTER_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.ENTER_PRESSED = true;
            input.ENTER_JUST_PRESSED = false;
        }
        else
        {
            input.ENTER_PRESSED = false;
            input.ENTER_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_TAB:
        if (action == GLFW_PRESS)
        {
            input.TAB_PRESSED = true;
            input.TAB_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.TAB_PRESSED = true;
            input.TAB_JUST_PRESSED = false;
        }
        else
        {
            input.TAB_PRESSED = false;
            input.TAB_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_BACKSPACE:
        if (action == GLFW_PRESS)
        {
            input.BACKSPACE_PRESSED = true;
            input.BACKSPACE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.BACKSPACE_PRESSED = true;
            input.BACKSPACE_JUST_PRESSED = false;
        }
        else
        {
            input.BACKSPACE_PRESSED = false;
            input.BACKSPACE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_INSERT:
        if (action == GLFW_PRESS)
        {
            input.INSERT_PRESSED = true;
            input.INSERT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.INSERT_PRESSED = true;
            input.INSERT_JUST_PRESSED = false;
        }
        else
        {
            input.INSERT_PRESSED = false;
            input.INSERT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_DELETE:
        if (action == GLFW_PRESS)
        {
            input.DELETE_PRESSED = true;
            input.DELETE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.DELETE_PRESSED = true;
            input.DELETE_JUST_PRESSED = false;
        }
        else
        {
            input.DELETE_PRESSED = false;
            input.DELETE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_RIGHT:
        if (action == GLFW_PRESS)
        {
            input.RIGHT_PRESSED = true;
            input.RIGHT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.RIGHT_PRESSED = true;
            input.RIGHT_JUST_PRESSED = false;
        }
        else
        {
            input.RIGHT_PRESSED = false;
            input.RIGHT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_LEFT:
        if (action == GLFW_PRESS)
        {
            input.LEFT_PRESSED = true;
            input.LEFT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.LEFT_PRESSED = true;
            input.LEFT_JUST_PRESSED = false;
        }
        else
        {
            input.LEFT_PRESSED = false;
            input.LEFT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_UP:
        if (action == GLFW_PRESS)
        {
            input.UP_PRESSED = true;
            input.UP_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.UP_PRESSED = true;
            input.UP_JUST_PRESSED = false;
        }
        else
        {
            input.UP_PRESSED = false;
            input.UP_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_DOWN:
        if (action == GLFW_PRESS)
        {
            input.DOWN_PRESSED = true;
            input.DOWN_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.DOWN_PRESSED = true;
            input.DOWN_JUST_PRESSED = false;
        }
        else
        {
            input.DOWN_PRESSED = false;
            input.DOWN_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_PAGE_UP:
        if (action == GLFW_PRESS)
        {
            input.PAGE_UP_PRESSED = true;
            input.PAGE_UP_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.PAGE_UP_PRESSED = true;
            input.PAGE_UP_JUST_PRESSED = false;
        }
        else
        {
            input.PAGE_UP_PRESSED = false;
            input.PAGE_UP_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_PAGE_DOWN:
        if (action == GLFW_PRESS)
        {
            input.PAGE_DOWN_PRESSED = true;
            input.PAGE_DOWN_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.PAGE_DOWN_PRESSED = true;
            input.PAGE_DOWN_JUST_PRESSED = false;
        }
        else
        {
            input.PAGE_DOWN_PRESSED = false;
            input.PAGE_DOWN_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_HOME:
        if (action == GLFW_PRESS)
        {
            input.HOME_PRESSED = true;
            input.HOME_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.HOME_PRESSED = true;
            input.HOME_JUST_PRESSED = false;
        }
        else
        {
            input.HOME_PRESSED = false;
            input.HOME_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_END:
        if (action == GLFW_PRESS)
        {
            input.END_PRESSED = true;
            input.END_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.END_PRESSED = true;
            input.END_JUST_PRESSED = false;
        }
        else
        {
            input.END_PRESSED = false;
            input.END_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_CAPS_LOCK:
        if (action == GLFW_PRESS)
        {
            input.CAPS_LOCK_PRESSED = true;
            input.CAPS_LOCK_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.CAPS_LOCK_PRESSED = true;
            input.CAPS_LOCK_JUST_PRESSED = false;
        }
        else
        {
            input.CAPS_LOCK_PRESSED = false;
            input.CAPS_LOCK_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_SCROLL_LOCK:
        if (action == GLFW_PRESS)
        {
            input.SCROLL_LOCK_PRESSED = true;
            input.SCROLL_LOCK_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.SCROLL_LOCK_PRESSED = true;
            input.SCROLL_LOCK_JUST_PRESSED = false;
        }
        else
        {
            input.SCROLL_LOCK_PRESSED = false;
            input.SCROLL_LOCK_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_NUM_LOCK:
        if (action == GLFW_PRESS)
        {
            input.NUM_LOCK_PRESSED = true;
            input.NUM_LOCK_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.NUM_LOCK_PRESSED = true;
            input.NUM_LOCK_JUST_PRESSED = false;
        }
        else
        {
            input.NUM_LOCK_PRESSED = false;
            input.NUM_LOCK_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_PRINT_SCREEN:
        if (action == GLFW_PRESS)
        {
            input.PRINT_SCREEN_PRESSED = true;
            input.PRINT_SCREEN_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.PRINT_SCREEN_PRESSED = true;
            input.PRINT_SCREEN_JUST_PRESSED = false;
        }
        else
        {
            input.PRINT_SCREEN_PRESSED = false;
            input.PRINT_SCREEN_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_PAUSE:
        if (action == GLFW_PRESS)
        {
            input.PAUSE_PRESSED = true;
            input.PAUSE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.PAUSE_PRESSED = true;
            input.PAUSE_JUST_PRESSED = false;
        }
        else
        {
            input.PAUSE_PRESSED = false;
            input.PAUSE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F1:
        if (action == GLFW_PRESS)
        {
            input.F1_PRESSED = true;
            input.F1_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F1_PRESSED = true;
            input.F1_JUST_PRESSED = false;
        }
        else
        {
            input.F1_PRESSED = false;
            input.F1_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F2:
        if (action == GLFW_PRESS)
        {
            input.F2_PRESSED = true;
            input.F2_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F2_PRESSED = true;
            input.F2_JUST_PRESSED = false;
        }
        else
        {
            input.F2_PRESSED = false;
            input.F2_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F3:
        if (action == GLFW_PRESS)
        {
            input.F3_PRESSED = true;
            input.F3_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F3_PRESSED = true;
            input.F3_JUST_PRESSED = false;
        }
        else
        {
            input.F3_PRESSED = false;
            input.F3_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F4:
        if (action == GLFW_PRESS)
        {
            input.F4_PRESSED = true;
            input.F4_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F4_PRESSED = true;
            input.F4_JUST_PRESSED = false;
        }
        else
        {
            input.F4_PRESSED = false;
            input.F4_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F5:
        if (action == GLFW_PRESS)
        {
            input.F5_PRESSED = true;
            input.F5_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F5_PRESSED = true;
            input.F5_JUST_PRESSED = false;
        }
        else
        {
            input.F5_PRESSED = false;
            input.F5_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F6:
        if (action == GLFW_PRESS)
        {
            input.F6_PRESSED = true;
            input.F6_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F6_PRESSED = true;
            input.F6_JUST_PRESSED = false;
        }
        else
        {
            input.F6_PRESSED = false;
            input.F6_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F7:
        if (action == GLFW_PRESS)
        {
            input.F7_PRESSED = true;
            input.F7_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F7_PRESSED = true;
            input.F7_JUST_PRESSED = false;
        }
        else
        {
            input.F7_PRESSED = false;
            input.F7_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F8:
        if (action == GLFW_PRESS)
        {
            input.F8_PRESSED = true;
            input.F8_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F8_PRESSED = true;
            input.F8_JUST_PRESSED = false;
        }
        else
        {
            input.F8_PRESSED = false;
            input.F8_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F9:
        if (action == GLFW_PRESS)
        {
            input.F9_PRESSED = true;
            input.F9_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F9_PRESSED = true;
            input.F9_JUST_PRESSED = false;
        }
        else
        {
            input.F9_PRESSED = false;
            input.F9_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F10:
        if (action == GLFW_PRESS)
        {
            input.F10_PRESSED = true;
            input.F10_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F10_PRESSED = true;
            input.F10_JUST_PRESSED = false;
        }
        else
        {
            input.F10_PRESSED = false;
            input.F10_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F11:
        if (action == GLFW_PRESS)
        {
            input.F11_PRESSED = true;
            input.F11_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F11_PRESSED = true;
            input.F11_JUST_PRESSED = false;
        }
        else
        {
            input.F11_PRESSED = false;
            input.F11_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_F12:
        if (action == GLFW_PRESS)
        {
            input.F12_PRESSED = true;
            input.F12_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.F12_PRESSED = true;
            input.F12_JUST_PRESSED = false;
        }
        else
        {
            input.F12_PRESSED = false;
            input.F12_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_0:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_0_PRESSED = true;
            input.KEY_PAD_0_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_0_PRESSED = true;
            input.KEY_PAD_0_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_0_PRESSED = false;
            input.KEY_PAD_0_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_1:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_1_PRESSED = true;
            input.KEY_PAD_1_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_1_PRESSED = true;
            input.KEY_PAD_1_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_1_PRESSED = false;
            input.KEY_PAD_1_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_2:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_2_PRESSED = true;
            input.KEY_PAD_2_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_2_PRESSED = true;
            input.KEY_PAD_2_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_2_PRESSED = false;
            input.KEY_PAD_2_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_3:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_3_PRESSED = true;
            input.KEY_PAD_3_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_3_PRESSED = true;
            input.KEY_PAD_3_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_3_PRESSED = false;
            input.KEY_PAD_3_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_4:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_4_PRESSED = true;
            input.KEY_PAD_4_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_4_PRESSED = true;
            input.KEY_PAD_4_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_4_PRESSED = false;
            input.KEY_PAD_4_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_5:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_5_PRESSED = true;
            input.KEY_PAD_5_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_5_PRESSED = true;
            input.KEY_PAD_5_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_5_PRESSED = false;
            input.KEY_PAD_5_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_6:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_6_PRESSED = true;
            input.KEY_PAD_6_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_6_PRESSED = true;
            input.KEY_PAD_6_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_6_PRESSED = false;
            input.KEY_PAD_6_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_7:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_7_PRESSED = true;
            input.KEY_PAD_7_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_7_PRESSED = true;
            input.KEY_PAD_7_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_7_PRESSED = false;
            input.KEY_PAD_7_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_8:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_8_PRESSED = true;
            input.KEY_PAD_8_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_8_PRESSED = true;
            input.KEY_PAD_8_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_8_PRESSED = false;
            input.KEY_PAD_8_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_9:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_9_PRESSED = true;
            input.KEY_PAD_9_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_9_PRESSED = true;
            input.KEY_PAD_9_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_9_PRESSED = false;
            input.KEY_PAD_9_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_DECIMAL:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_DECIMAL_PRESSED = true;
            input.KEY_PAD_DECIMAL_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_DECIMAL_PRESSED = true;
            input.KEY_PAD_DECIMAL_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_DECIMAL_PRESSED = false;
            input.KEY_PAD_DECIMAL_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_DIVIDE:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_DIVIDE_PRESSED = true;
            input.KEY_PAD_DIVIDE_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_DIVIDE_PRESSED = true;
            input.KEY_PAD_DIVIDE_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_DIVIDE_PRESSED = false;
            input.KEY_PAD_DIVIDE_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_MULTIPLY:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_MULTIPLY_PRESSED = true;
            input.KEY_PAD_MULTIPLY_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_MULTIPLY_PRESSED = true;
            input.KEY_PAD_MULTIPLY_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_MULTIPLY_PRESSED = false;
            input.KEY_PAD_MULTIPLY_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_SUBTRACT:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_SUBTRACT_PRESSED = true;
            input.KEY_PAD_SUBTRACT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_SUBTRACT_PRESSED = true;
            input.KEY_PAD_SUBTRACT_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_SUBTRACT_PRESSED = false;
            input.KEY_PAD_SUBTRACT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_ADD:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_ADD_PRESSED = true;
            input.KEY_PAD_ADD_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_ADD_PRESSED = true;
            input.KEY_PAD_ADD_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_ADD_PRESSED = false;
            input.KEY_PAD_ADD_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_ENTER:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_ENTER_PRESSED = true;
            input.KEY_PAD_ENTER_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_ENTER_PRESSED = true;
            input.KEY_PAD_ENTER_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_ENTER_PRESSED = false;
            input.KEY_PAD_ENTER_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_KP_EQUAL:
        if (action == GLFW_PRESS)
        {
            input.KEY_PAD_EQUAL_PRESSED = true;
            input.KEY_PAD_EQUAL_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.KEY_PAD_EQUAL_PRESSED = true;
            input.KEY_PAD_EQUAL_JUST_PRESSED = false;
        }
        else
        {
            input.KEY_PAD_EQUAL_PRESSED = false;
            input.KEY_PAD_EQUAL_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_LEFT_SHIFT:
        if (action == GLFW_PRESS)
        {
            input.LEFT_SHIFT_PRESSED = true;
            input.LEFT_SHIFT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.LEFT_SHIFT_PRESSED = true;
            input.LEFT_SHIFT_JUST_PRESSED = false;
        }
        else
        {
            input.LEFT_SHIFT_PRESSED = false;
            input.LEFT_SHIFT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_LEFT_CONTROL:
        if (action == GLFW_PRESS)
        {
            input.LEFT_CONTROL_PRESSED = true;
            input.LEFT_CONTROL_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.LEFT_CONTROL_PRESSED = true;
            input.LEFT_CONTROL_JUST_PRESSED = false;
        }
        else
        {
            input.LEFT_CONTROL_PRESSED = false;
            input.LEFT_CONTROL_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_LEFT_ALT:
        if (action == GLFW_PRESS)
        {
            input.LEFT_ALT_PRESSED = true;
            input.LEFT_ALT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.LEFT_ALT_PRESSED = true;
            input.LEFT_ALT_JUST_PRESSED = false;
        }
        else
        {
            input.LEFT_ALT_PRESSED = false;
            input.LEFT_ALT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_LEFT_SUPER:
        if (action == GLFW_PRESS)
        {
            input.LEFT_SUPER_PRESSED = true;
            input.LEFT_SUPER_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.LEFT_SUPER_PRESSED = true;
            input.LEFT_SUPER_JUST_PRESSED = false;
        }
        else
        {
            input.LEFT_SUPER_PRESSED = false;
            input.LEFT_SUPER_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_RIGHT_SHIFT:
        if (action == GLFW_PRESS)
        {
            input.RIGHT_SHIFT_PRESSED = true;
            input.RIGHT_SHIFT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.RIGHT_SHIFT_PRESSED = true;
            input.RIGHT_SHIFT_JUST_PRESSED = false;
        }
        else
        {
            input.RIGHT_SHIFT_PRESSED = false;
            input.RIGHT_SHIFT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_RIGHT_CONTROL:
        if (action == GLFW_PRESS)
        {
            input.RIGHT_CONTROL_PRESSED = true;
            input.RIGHT_CONTROL_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.RIGHT_CONTROL_PRESSED = true;
            input.RIGHT_CONTROL_JUST_PRESSED = false;
        }
        else
        {
            input.RIGHT_CONTROL_PRESSED = false;
            input.RIGHT_CONTROL_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_RIGHT_ALT:
        if (action == GLFW_PRESS)
        {
            input.RIGHT_ALT_PRESSED = true;
            input.RIGHT_ALT_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.RIGHT_ALT_PRESSED = true;
            input.RIGHT_ALT_JUST_PRESSED = false;
        }
        else
        {
            input.RIGHT_ALT_PRESSED = false;
            input.RIGHT_ALT_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_RIGHT_SUPER:
        if (action == GLFW_PRESS)
        {
            input.RIGHT_SUPER_PRESSED = true;
            input.RIGHT_SUPER_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.RIGHT_SUPER_PRESSED = true;
            input.RIGHT_SUPER_JUST_PRESSED = false;
        }
        else
        {
            input.RIGHT_SUPER_PRESSED = false;
            input.RIGHT_SUPER_JUST_PRESSED = false;
        }
        break;
    case GLFW_KEY_MENU:
        if (action == GLFW_PRESS)
        {
            input.MENU_PRESSED = true;
            input.MENU_JUST_PRESSED = true;
        }
        else if (action == GLFW_REPEAT)
        {
            input.MENU_PRESSED = true;
            input.MENU_JUST_PRESSED = false;
        }
        else
        {
            input.MENU_PRESSED = false;
            input.MENU_JUST_PRESSED = false;
        }
        break;
    default:
        break;
    }
    if (info.scene)
    {
        // reverse iterate through overlay callbacks first
        bool handled = false;
        for (auto overlay_it = info.scene->overlay_rbegin(); overlay_it != info.scene->overlay_rend(); overlay_it++)
        {
            Overlay *overlay = overlay_it->get();
            // reverse iterate through overlay widgets
            for (auto widget_it = overlay->widget_rbegin(); widget_it != overlay->widget_rend(); widget_it++)
            {
                Widget *widget = widget_it->get();
                auto controls_systems = widget->get_components<IControlsSystem>();
                // reverse iterate through widget controls callbacks
                for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
                {
                    handled = (*controls_it)->on_key(widget, input);
                    if (handled)
                    {
                        return;
                    }
                }
            }
            auto controls_systems = overlay->get_components<IControlsSystem>();
            // reverse iterate through overlay controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_key(overlay, input);
                if (handled)
                {
                    return;
                }
            }
        }
        // if event is not handled by overlays, callback to the scene
        // reverse iterate through scene entities
        for (auto entity_it = info.scene->entity_rbegin(); entity_it != info.scene->entity_rend(); entity_it++)
        {
            Entity *entity = entity_it->get();
            auto controls_systems = entity->get_components<IControlsSystem>();
            // reverse iterate through entity controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_key(entity, input);
                if (handled)
                {
                    return;
                }
            }
        }
        auto controls_systems = info.scene->get_components<IControlsSystem>();
        // reverse iterate through scene controls callbacks
        for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
        {
            handled = (*controls_it)->on_key(info.scene, input);
            if (handled)
            {
                return;
            }
        }
    }
}

void GLRenderer::glfw_onMouseButton(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        input.mouse_button.set(0, 1);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        input.mouse_button.set(0, 0);
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        input.mouse_button.set(1, 1);
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        input.mouse_button.set(1, 0);
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        input.mouse_button.set(2, 1);
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        input.mouse_button.set(2, 0);
    }
    if (info.scene)
    {
        // reverse iterate through overlay callbacks first
        bool handled = false;
        for (auto overlay_it = info.scene->overlay_rbegin(); overlay_it != info.scene->overlay_rend(); overlay_it++)
        {
            Overlay *overlay = overlay_it->get();
            // reverse iterate through overlay widgets
            for (auto widget_it = overlay->widget_rbegin(); widget_it != overlay->widget_rend(); widget_it++)
            {
                Widget *widget = widget_it->get();
                auto controls_systems = widget->get_components<IControlsSystem>();
                // reverse iterate through widget controls callbacks
                for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
                {
                    handled = (*controls_it)->on_mouse_button(widget, input);
                    if (handled)
                    {
                        return;
                    }
                }
            }
            auto controls_systems = overlay->get_components<IControlsSystem>();
            // reverse iterate through overlay controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_mouse_button(overlay, input);
                if (handled)
                {
                    return;
                }
            }
        }
        // if event is not handled by overlays, callback to the scene
        // reverse iterate through scene entities
        for (auto entity_it = info.scene->entity_rbegin(); entity_it != info.scene->entity_rend(); entity_it++)
        {
            Entity *entity = entity_it->get();
            auto controls_systems = entity->get_components<IControlsSystem>();
            // reverse iterate through entity controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_mouse_button(entity, input);
                if (handled)
                {
                    return;
                }
            }
        }
        auto controls_systems = info.scene->get_components<IControlsSystem>();
        // reverse iterate through scene controls callbacks
        for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
        {
            handled = (*controls_it)->on_mouse_button(info.scene, input);
            if (handled)
            {
                return;
            }
        }
    }
}

void GLRenderer::glfw_onMouseMove(GLFWwindow *window, double x, double y)
{
    glm::ivec2 old_pos = input.mouse_pos;
    input.mouse_pos = glm::ivec2(x, y);
    input.mouse_vel = glm::ivec2(x, y) - old_pos;
    std::cout << "Mouse Vel: " << input.mouse_vel.x << ", " << input.mouse_vel.y << std::endl;
    if (info.scene)
    {
        // reverse iterate through overlay callbacks first
        bool handled = false;
        for (auto overlay_it = info.scene->overlay_rbegin(); overlay_it != info.scene->overlay_rend(); overlay_it++)
        {
            Overlay *overlay = overlay_it->get();
            // reverse iterate through overlay widgets
            for (auto widget_it = overlay->widget_rbegin(); widget_it != overlay->widget_rend(); widget_it++)
            {
                Widget *widget = widget_it->get();
                auto controls_systems = widget->get_components<IControlsSystem>();
                // reverse iterate through widget controls callbacks
                for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
                {
                    handled = (*controls_it)->on_mouse_move(widget, input);
                    if (handled)
                    {
                        // mouse velocity is always zero outside of mouse move callbacks
                        input.mouse_vel = glm::ivec2(0, 0);
                        return;
                    }
                }
            }
            auto controls_systems = overlay->get_components<IControlsSystem>();
            // reverse iterate through overlay controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_mouse_move(overlay, input);
                if (handled)
                {
                    // mouse velocity is always zero outside of mouse move callbacks
                    input.mouse_vel = glm::ivec2(0, 0);
                    return;
                }
            }
        }
        // if event is not handled by overlays, callback to the scene
        // reverse iterate through scene entities
        for (auto entity_it = info.scene->entity_rbegin(); entity_it != info.scene->entity_rend(); entity_it++)
        {
            Entity *entity = entity_it->get();
            auto controls_systems = entity->get_components<IControlsSystem>();
            // reverse iterate through entity controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_mouse_move(entity, input);
                if (handled)
                {
                    // mouse velocity is always zero outside of mouse move callbacks
                    input.mouse_vel = glm::ivec2(0, 0);
                    return;
                }
            }
        }
        auto controls_systems = info.scene->get_components<IControlsSystem>();
        // reverse iterate through scene controls callbacks
        for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
        {
            handled = (*controls_it)->on_mouse_move(info.scene, input);
            if (handled)
            {
                // mouse velocity is always zero outside of mouse move callbacks
                input.mouse_vel = glm::ivec2(0, 0);
                return;
            }
        }
    }
    // mouse velocity is always zero outside of mouse move callbacks
    input.mouse_vel = glm::ivec2(0, 0);
}

void GLRenderer::glfw_onMouseWheel(GLFWwindow *window, double xoffset, double yoffset)
{
    if (yoffset > 0)
    {
        input.mouse_scroll = 1;
    }
    if (yoffset < 0)
    {
        input.mouse_scroll = -1;
    }
    if (info.scene)
    {
        // reverse iterate through overlay callbacks first
        bool handled = false;
        for (auto overlay_it = info.scene->overlay_rbegin(); overlay_it != info.scene->overlay_rend(); overlay_it++)
        {
            Overlay *overlay = overlay_it->get();
            // reverse iterate through overlay widgets
            for (auto widget_it = overlay->widget_rbegin(); widget_it != overlay->widget_rend(); widget_it++)
            {
                Widget *widget = widget_it->get();
                auto controls_systems = widget->get_components<IControlsSystem>();
                // reverse iterate through widget controls callbacks
                for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
                {
                    handled = (*controls_it)->on_mouse_wheel(widget, input);
                    if (handled)
                    {
                        // mouse scroll is always 0 outside of mouse scroll callbacks
                        input.mouse_scroll = 0;
                        return;
                    }
                }
            }
            auto controls_systems = overlay->get_components<IControlsSystem>();
            // reverse iterate through overlay controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_mouse_wheel(overlay, input);
                if (handled)
                {
                    // mouse scroll is always 0 outside of mouse scroll callbacks
                    input.mouse_scroll = 0;
                    return;
                }
            }
        }
        // if event is not handled by overlays, callback to the scene
        // reverse iterate through scene entities
        for (auto entity_it = info.scene->entity_rbegin(); entity_it != info.scene->entity_rend(); entity_it++)
        {
            Entity *entity = entity_it->get();
            auto controls_systems = entity->get_components<IControlsSystem>();
            // reverse iterate through entity controls callbacks
            for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
            {
                handled = (*controls_it)->on_mouse_wheel(entity, input);
                if (handled)
                {
                    // mouse scroll is always 0 outside of mouse scroll callbacks
                    input.mouse_scroll = 0;
                    return;
                }
            }
        }
        auto controls_systems = info.scene->get_components<IControlsSystem>();
        // reverse iterate through scene controls callbacks
        for (auto controls_it = controls_systems.rbegin(); controls_it != controls_systems.rend(); controls_it++)
        {
            handled = (*controls_it)->on_mouse_wheel(info.scene, input);
            if (handled)
            {
                // mouse scroll is always 0 outside of mouse scroll callbacks
                input.mouse_scroll = 0;
                return;
            }
        }
    }
    // mouse scroll is always 0 outside of mouse scroll callbacks
    input.mouse_scroll = 0;
}

// Initialize static variable for the window
GLFWwindow *GLRenderer::window = nullptr;

} // namespace mare
