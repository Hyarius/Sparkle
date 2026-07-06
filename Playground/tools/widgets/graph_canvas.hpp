#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <sparkle.hpp>

namespace pg::tools
{
	class GraphCanvas : public spk::Widget
	{
	public:
		struct Node
		{
			std::string id;
			std::string label;
			spk::Vector2 position{};
			spk::Vector2 size{1.25f, 0.65f};
			spk::Color color{0.3f, 0.45f, 0.7f, 1.0f};
			bool root = false;
		};

		struct Edge
		{
			std::string first;
			std::string second;
		};

	private:
		static constexpr float UnitPixels = 130.0f;
		std::vector<Node> _nodes;
		std::vector<Edge> _edges;
		std::vector<std::string> _selection;
		std::unordered_map<std::string, std::unique_ptr<spk::TextLabel>> _labels;
		spk::Vector2 _pan{};
		float _zoom = 1.0f;
		std::optional<std::string> _draggedNode;
		std::optional<std::string> _linkSource;
		std::optional<spk::Vector2Int> _boxStart;
		bool _panning = false;

		std::function<void(const std::string &)> _selectionCallback;
		std::function<void(const std::string &, const spk::Vector2 &)> _moveCallback;
		std::function<void(const std::string &, const std::string &)> _linkCallback;
		std::function<void(const spk::Vector2 &)> _addCallback;
		std::function<void(const std::string &)> _deleteCallback;

		[[nodiscard]] spk::Vector2Int _localMouse(const spk::Vector2Int &p_absolute) const;
		[[nodiscard]] spk::Rect2D _nodeRect(const Node &p_node) const;
		[[nodiscard]] Node *_findNode(const std::string &p_id);
		[[nodiscard]] const Node *_findNode(const std::string &p_id) const;
		void _refreshLabels();
		void _refreshLabelGeometry();
		void _selectOnly(const std::string &p_id);

	protected:
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;
		void _onGeometryChange() override;
		void _onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) override;
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;

	public:
		GraphCanvas(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void setGraph(std::vector<Node> p_nodes, std::vector<Edge> p_edges);
		[[nodiscard]] spk::Vector2 canvasToScreen(const spk::Vector2 &p_canvas) const;
		[[nodiscard]] spk::Vector2 screenToCanvas(const spk::Vector2 &p_screen) const;
		[[nodiscard]] std::optional<std::string> hitTest(const spk::Vector2Int &p_localPoint) const;
		void frameAll();

		void onSelection(std::function<void(const std::string &)> p_callback);
		void onNodeMoved(std::function<void(const std::string &, const spk::Vector2 &)> p_callback);
		void onLinkToggled(std::function<void(const std::string &, const std::string &)> p_callback);
		void onNodeAdded(std::function<void(const spk::Vector2 &)> p_callback);
		void onNodeDeleted(std::function<void(const std::string &)> p_callback);

		[[nodiscard]] const std::vector<Node> &nodes() const noexcept;
		[[nodiscard]] const std::vector<Edge> &edges() const noexcept;
		[[nodiscard]] const std::vector<std::string> &selection() const noexcept;
		[[nodiscard]] float zoom() const noexcept;
		[[nodiscard]] const spk::Vector2 &pan() const noexcept;
	};
}
