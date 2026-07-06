#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <sparkle.hpp>

namespace pg::tools
{
	class Viewport3D : public spk::GameEngineWidget
	{
	public:
		using SceneBuilder = std::function<void(Viewport3D &)>;

	private:
		spk::Entity3D _cameraEntity;
		spk::Camera3D *_camera = nullptr;
		std::vector<std::unique_ptr<spk::Entity3D>> _sceneEntities;
		SceneBuilder _sceneBuilder;
		spk::Vector3 _target{0.5f, 0.5f, 0.5f};
		float _yaw = 45.0f;
		float _pitch = 32.0f;
		float _distance = 5.0f;

		void _updateCamera();

	protected:
		void _onGeometryChange() override;
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;
		void _onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event) override;

	public:
		Viewport3D(const std::string &p_name, spk::Widget *p_parent = nullptr);
		~Viewport3D() override;

		void setSceneBuilder(SceneBuilder p_builder);
		void rebuildScene();
		void clearScene();
		spk::Entity3D &createEntity();
		void frame(const spk::Vector3 &p_target, float p_distance);
		void pan(const spk::Vector2 &p_pixels);
		[[nodiscard]] spk::Camera3D &camera() noexcept;
	};
}
