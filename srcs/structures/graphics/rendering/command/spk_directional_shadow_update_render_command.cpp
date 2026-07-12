#include "structures/graphics/rendering/command/spk_directional_shadow_update_render_command.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_texture.hpp"
namespace spk { DirectionalShadowUpdateRenderCommand::DirectionalShadowUpdateRenderCommand(std::shared_ptr<const UniformBufferObject> data, std::array<std::shared_ptr<const FrameBufferObject>,4> targets) : _data(std::move(data)), _targets(std::move(targets)) {} void DirectionalShadowUpdateRenderCommand::execute(RenderContext &context) { if (_data) _data->activate(context); for (std::size_t i=0;i<_targets.size();++i) { glActiveTexture(GL_TEXTURE0 + 8 + static_cast<GLenum>(i)); const auto &target=_targets[i]; if (target && target->tryDepthAttachment()) glBindTexture(GL_TEXTURE_2D, target->tryDepthAttachment()->gpu(context).id()); else glBindTexture(GL_TEXTURE_2D, 0); } } }
