#pragma once
#include <memory>
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"
namespace spk { class DrawTextureMeshShadowRenderCommand final : public RenderCommand { std::shared_ptr<const TextureMesh3D> _mesh; std::shared_ptr<UniformBufferObject> _model; UniformBufferObject _shadow; public: DrawTextureMeshShadowRenderCommand(std::shared_ptr<const TextureMesh3D>, std::shared_ptr<UniformBufferObject>, const Matrix4x4&); void execute(RenderContext&) override; }; }
