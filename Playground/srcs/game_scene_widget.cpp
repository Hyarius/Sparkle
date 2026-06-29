#include "game_scene_widget.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

#include "components/animation2d.hpp"
#include "components/camera2d.hpp"
#include "components/player_controller.hpp"
#include "components/sprite_renderer2d.hpp"
#include "components/transform2d.hpp"
#include "logics/animation_logic.hpp"
#include "logics/player_control_logic.hpp"
#include "logics/sprite_render_logic.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"

#ifndef PG_RESOURCE_DIR
#	define PG_RESOURCE_DIR "."
#endif

namespace
{
	enum OverlayRow : std::size_t
	{
		Position = 0,
		Size,
		Rotation,
		Animation,
		SpriteCell,
		Sprites,
		Polygons,
		UpdateTime,
		RenderTime,
		DeltaTime,
		RowCount
	};

	[[nodiscard]] long long nowNs()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
				   std::chrono::steady_clock::now().time_since_epoch())
			.count();
	}

	[[nodiscard]] std::string formatVector(const spk::Vector2 &p_value)
	{
		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "(%.2f, %.2f)", p_value.x, p_value.y);
		return buffer;
	}

	[[nodiscard]] std::string formatFloat(float p_value, const char *p_suffix = "")
	{
		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "%.2f%s", p_value, p_suffix);
		return buffer;
	}

	[[nodiscard]] std::string narrow(const std::wstring &p_value)
	{
		return std::string(p_value.begin(), p_value.end());
	}

	[[nodiscard]] pg::Animation2D makeRowAnimation(std::uint32_t p_row, std::uint32_t p_columnCount)
	{
		pg::Animation2D animation;
		animation.loop = true;
		animation.frameDuration = spk::Duration(120.0L, spk::TimeUnit::Millisecond);
		for (std::uint32_t column = 0; column < p_columnCount; ++column)
		{
			animation.frames.push_back({column, p_row});
		}
		return animation;
	}
}

namespace pg
{
	GameSceneWidget::GameSceneWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::GameEngineWidget(p_name, p_parent),
		_overlay(p_name + "/DebugOverlay", this)
	{
		_buildScene();
		_configureOverlay();
		activate();
	}

	void GameSceneWidget::_buildScene()
	{
		const std::filesystem::path spriteSheetPath =
			std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "spriteSheet.png";

		_spriteSheet.loadFromFile(spriteSheetPath, {4u, 3u});
		_spriteMesh = spk::PrimitiveObject::CreateSquare({-0.75f, -0.75f}, {1.5f, 1.5f}, {0.0f, 0.0f}, {1.0f, 1.0f});

		spk::GameEngine &engine = gameEngine();

		engine.add<PlayerControlLogic>().setPriority(20);
		engine.add<AnimationLogic>().setPriority(10);
		engine.add<SpriteRenderLogic>();

		Camera2D &camera = _cameraEntity.addComponent<Camera2D>();
		camera.setPixelsPerUnit({64.0f, 64.0f});
		camera.makeMain();

		_player.transform().setPosition({0.0f, 0.0f});
		SpriteRenderer2D &playerSprite = _player.addComponent<SpriteRenderer2D>();
		playerSprite.setSpriteSheet(&_spriteSheet);
		playerSprite.setMesh(&_spriteMesh);
		AnimationController2D &playerAnimation = _player.addComponent<AnimationController2D>();
		playerAnimation.addAnimation(L"down", makeRowAnimation(0, 4));
		playerAnimation.addAnimation(L"side", makeRowAnimation(1, 4));
		playerAnimation.addAnimation(L"up", makeRowAnimation(2, 4));
		playerAnimation.play(L"down");
		playerAnimation.stop();
		_player.addComponent<PlayerController>(4.0f);
		engine.addEntity(&_player);

		_objectA.transform().setPosition({-3.0f, 1.0f});
		SpriteRenderer2D &spriteA = _objectA.addComponent<SpriteRenderer2D>();
		spriteA.setSpriteSheet(&_spriteSheet);
		spriteA.setMesh(&_spriteMesh);
		spriteA.setSprite(_spriteSheet.sprite({0u, 0u}));
		engine.addEntity(&_objectA);

		_objectB.transform().setPosition({3.0f, -1.0f});
		SpriteRenderer2D &spriteB = _objectB.addComponent<SpriteRenderer2D>();
		spriteB.setSpriteSheet(&_spriteSheet);
		spriteB.setMesh(&_spriteMesh);
		AnimationController2D &animationB = _objectB.addComponent<AnimationController2D>();
		animationB.addAnimation(L"idle", makeRowAnimation(2, 4));
		animationB.play(L"idle");
		engine.addEntity(&_objectB);
	}

	void GameSceneWidget::_configureOverlay()
	{
		_overlay.configureRows(RowCount, 2);
		_overlay.setMaxGlyphSize(16);
		_overlay.setFontOutlineSize(1);

		_overlay.setText(Position, 0, "Player position");
		_overlay.setText(Size, 0, "Player sprite UV size");
		_overlay.setText(Rotation, 0, "Player rotation");
		_overlay.setText(Animation, 0, "Player animation");
		_overlay.setText(SpriteCell, 0, "Player sprite UV anchor");
		_overlay.setText(Sprites, 0, "Sprites rendered");
		_overlay.setText(Polygons, 0, "Polygons rendered");
		_overlay.setText(UpdateTime, 0, "Update duration");
		_overlay.setText(RenderTime, 0, "Render duration");
		_overlay.setText(DeltaTime, 0, "Frame delta");
	}

	void GameSceneWidget::_onGeometryChange()
	{
		spk::GameEngineWidget::_onGeometryChange();

		if (Camera2D *camera = Camera2D::mainCamera(); camera != nullptr)
		{
			camera->setViewport(geometry());
		}

		const std::uint32_t overlayWidth = std::min<std::uint32_t>(geometry().size.x, 360u);
		_overlay.setGeometry(spk::Rect2D({8, 8}, {overlayWidth, 26u * RowCount}));
	}

	void GameSceneWidget::_onUpdate(const spk::UpdateTick &p_tick)
	{
		const long long start = nowNs();
		spk::GameEngineWidget::_onUpdate(p_tick);
		_updateDurationNs.store(nowNs() - start, std::memory_order_relaxed);

		invalidateRenderUnit();

		_refreshOverlay(p_tick);
	}

	spk::RenderUnit GameSceneWidget::_buildRenderUnit() const
	{
		const long long start = nowNs();
		spk::RenderUnit unit = spk::GameEngineWidget::_buildRenderUnit();
		_renderDurationNs.store(nowNs() - start, std::memory_order_relaxed);

		_spriteCount.store(SpriteRenderLogic::lastSpriteCount(), std::memory_order_relaxed);
		_polygonCount.store(SpriteRenderLogic::lastPolygonCount(), std::memory_order_relaxed);
		return unit;
	}

	void GameSceneWidget::_refreshOverlay(const spk::UpdateTick &p_tick)
	{
		const Transform2D &transform = _player.transform();
		_overlay.setText(Position, 1, formatVector(transform.position()));
		_overlay.setText(Rotation, 1, formatFloat(transform.rotation(), " deg"));

		if (const SpriteRenderer2D *sprite = _player.component<SpriteRenderer2D>(); sprite != nullptr)
		{
			_overlay.setText(Size, 1, formatVector(sprite->sprite().size));
			_overlay.setText(SpriteCell, 1, formatVector(sprite->sprite().anchor));
		}

		if (const AnimationController2D *animation = _player.component<AnimationController2D>(); animation != nullptr)
		{
			std::string name = animation->hasCurrent() ? narrow(animation->currentName()) : "none";
			name += animation->isPlaying() ? " (playing)" : " (idle)";
			_overlay.setText(Animation, 1, name);
		}

		_overlay.setText(Sprites, 1, std::to_string(_spriteCount.load(std::memory_order_relaxed)));
		_overlay.setText(Polygons, 1, std::to_string(_polygonCount.load(std::memory_order_relaxed)));
		_overlay.setText(UpdateTime, 1, std::to_string(_updateDurationNs.load(std::memory_order_relaxed)) + " ns");
		_overlay.setText(RenderTime, 1, std::to_string(_renderDurationNs.load(std::memory_order_relaxed)) + " ns");
		_overlay.setText(DeltaTime, 1, std::to_string(p_tick.deltaTime.milliseconds()) + " ms");
	}
}
