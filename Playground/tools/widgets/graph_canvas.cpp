#include "tools/widgets/graph_canvas.hpp"

#include "structures/graphics/rendering/command/spk_color_rectangle_render_command.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace pg::tools
{
	GraphCanvas::GraphCanvas(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		setMinimalSize({320, 240});
	}

	spk::Vector2Int GraphCanvas::_localMouse(const spk::Vector2Int &p_absolute) const
	{
		return p_absolute - absoluteGeometry().anchor;
	}

	spk::Vector2 GraphCanvas::canvasToScreen(const spk::Vector2 &p_canvas) const
	{
		return {
			static_cast<float>(geometry().width()) * 0.5f + (p_canvas.x * UnitPixels + _pan.x) * _zoom,
			static_cast<float>(geometry().height()) * 0.5f + (p_canvas.y * UnitPixels + _pan.y) * _zoom};
	}

	spk::Vector2 GraphCanvas::screenToCanvas(const spk::Vector2 &p_screen) const
	{
		return {
			((p_screen.x - static_cast<float>(geometry().width()) * 0.5f) / _zoom - _pan.x) / UnitPixels,
			((p_screen.y - static_cast<float>(geometry().height()) * 0.5f) / _zoom - _pan.y) / UnitPixels};
	}

	spk::Rect2D GraphCanvas::_nodeRect(const Node &p_node) const
	{
		const spk::Vector2 center = canvasToScreen(p_node.position);
		const spk::Vector2 size = p_node.size * (UnitPixels * _zoom);
		return {
			{static_cast<int>(std::lround(center.x - size.x * 0.5f)), static_cast<int>(std::lround(center.y - size.y * 0.5f))},
			{static_cast<unsigned int>(std::max(1.0f, size.x)), static_cast<unsigned int>(std::max(1.0f, size.y))}};
	}

	GraphCanvas::Node *GraphCanvas::_findNode(const std::string &p_id)
	{
		const auto iterator = std::ranges::find(_nodes, p_id, &Node::id);
		return iterator == _nodes.end() ? nullptr : &*iterator;
	}

	const GraphCanvas::Node *GraphCanvas::_findNode(const std::string &p_id) const
	{
		const auto iterator = std::ranges::find(_nodes, p_id, &Node::id);
		return iterator == _nodes.end() ? nullptr : &*iterator;
	}

	void GraphCanvas::_refreshLabels()
	{
		_labels.clear();
		for (const Node &node : _nodes)
		{
			auto label = std::make_unique<spk::TextLabel>(name() + "/NodeLabel/" + node.id, node.label, this);
			label->setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
			label->activate();
			_labels.emplace(node.id, std::move(label));
		}
		_refreshLabelGeometry();
	}

	void GraphCanvas::_refreshLabelGeometry()
	{
		for (const Node &node : _nodes)
		{
			if (auto iterator = _labels.find(node.id); iterator != _labels.end())
			{
				iterator->second->setGeometry(_nodeRect(node).shrink({5, 5}));
			}
		}
	}

	void GraphCanvas::setGraph(std::vector<Node> p_nodes, std::vector<Edge> p_edges)
	{
		_nodes = std::move(p_nodes);
		_edges = std::move(p_edges);
		std::erase_if(_selection, [this](const std::string &p_id) { return _findNode(p_id) == nullptr; });
		_refreshLabels();
		invalidateRenderUnit();
	}

	std::optional<std::string> GraphCanvas::hitTest(const spk::Vector2Int &p_localPoint) const
	{
		for (auto iterator = _nodes.rbegin(); iterator != _nodes.rend(); ++iterator)
		{
			if (_nodeRect(*iterator).contains(p_localPoint)) return iterator->id;
		}
		return std::nullopt;
	}

	void GraphCanvas::_selectOnly(const std::string &p_id)
	{
		_selection = {p_id};
		if (_selectionCallback) _selectionCallback(p_id);
		invalidateRenderUnit();
	}

	spk::RenderUnit GraphCanvas::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;
		builder.emplace<spk::ColorRectangleRenderCommand>(geometry().atOrigin(), spk::Color{0.08f, 0.09f, 0.12f, 1.0f});
		for (const Edge &edge : _edges)
		{
			const Node *first = _findNode(edge.first);
			const Node *second = _findNode(edge.second);
			if (first == nullptr || second == nullptr) continue;
			const spk::Vector2 a = canvasToScreen(first->position);
			const spk::Vector2 b = canvasToScreen(second->position);
			const int ax = static_cast<int>(std::lround(a.x));
			const int ay = static_cast<int>(std::lround(a.y));
			const int bx = static_cast<int>(std::lround(b.x));
			const int by = static_cast<int>(std::lround(b.y));
			const spk::Color color{0.5f, 0.55f, 0.65f, 1.0f};
			builder.emplace<spk::ColorRectangleRenderCommand>(
				spk::Rect2D(std::min(ax, bx), ay - 1, static_cast<std::size_t>(std::max(2, std::abs(bx - ax))), 3), color);
			builder.emplace<spk::ColorRectangleRenderCommand>(
				spk::Rect2D(bx - 1, std::min(ay, by), 3, static_cast<std::size_t>(std::max(2, std::abs(by - ay)))), color);
		}
		for (const Node &node : _nodes)
		{
			const spk::Rect2D rect = _nodeRect(node);
			if (node.root)
				builder.emplace<spk::ColorRectangleRenderCommand>(
					spk::Rect2D(rect.x() - 4, rect.y() - 4, rect.width() + 8u, rect.height() + 8u),
					spk::Color{1.0f, 0.7f, 0.15f, 1.0f});
			if (std::ranges::contains(_selection, node.id))
				builder.emplace<spk::ColorRectangleRenderCommand>(
					spk::Rect2D(rect.x() - 2, rect.y() - 2, rect.width() + 4u, rect.height() + 4u),
					spk::Color{0.95f, 0.95f, 1.0f, 1.0f});
			builder.emplace<spk::ColorRectangleRenderCommand>(rect, node.color);
		}
		if (_boxStart.has_value())
		{
			// The active selection rectangle is intentionally subtle; the final selection
			// is computed from the release point in the mouse handler.
			builder.emplace<spk::ColorRectangleRenderCommand>(
				spk::Rect2D(*_boxStart, {3, 3}), spk::Color{0.8f, 0.85f, 1.0f, 1.0f});
		}
		return builder.build();
	}

	void GraphCanvas::_onGeometryChange()
	{
		_refreshLabelGeometry();
		invalidateRenderUnit();
	}

	void GraphCanvas::_onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event)
	{
		if (!absoluteGeometry().contains(p_event.device().position)) return;
		const spk::Vector2Int local = _localMouse(p_event.device().position);
		const spk::Vector2 before = screenToCanvas(static_cast<spk::Vector2>(local));
		_zoom = std::clamp(_zoom * (p_event->delta.y > 0 ? 1.12f : 0.89f), 0.25f, 3.0f);
		const spk::Vector2 after = screenToCanvas(static_cast<spk::Vector2>(local));
		_pan += (after - before) * (UnitPixels);
		_refreshLabelGeometry();
		invalidateRenderUnit();
		p_event.consume();
	}

	void GraphCanvas::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		if (!absoluteGeometry().contains(p_event.device().position)) return;
		const spk::Vector2Int local = _localMouse(p_event.device().position);
		const std::optional<std::string> hit = hitTest(local);
		if (p_event->button == spk::Mouse::Left)
		{
			if (hit.has_value()) { _selectOnly(*hit); _draggedNode = *hit; }
			else { _selection.clear(); _boxStart = local; invalidateRenderUnit(); }
			p_event.consume();
		}
		else if (p_event->button == spk::Mouse::Middle)
		{
			_panning = true;
			p_event.consume();
		}
		else if (p_event->button == spk::Mouse::Right)
		{
			if (hit.has_value()) _linkSource = *hit;
			else if (_addCallback) _addCallback(screenToCanvas(static_cast<spk::Vector2>(local)));
			p_event.consume();
		}
	}

	void GraphCanvas::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		const spk::Vector2Int local = _localMouse(p_event.device().position);
		if (p_event->button == spk::Mouse::Left)
		{
			_draggedNode.reset();
			if (_boxStart.has_value())
			{
				const spk::Vector2Int first = *_boxStart;
				const spk::Rect2D box(
					{std::min(first.x, local.x), std::min(first.y, local.y)},
					{static_cast<unsigned int>(std::abs(first.x - local.x)), static_cast<unsigned int>(std::abs(first.y - local.y))});
				_selection.clear();
				for (const Node &node : _nodes) if (box.contains(_nodeRect(node))) _selection.push_back(node.id);
				if (_selection.size() == 1 && _selectionCallback) _selectionCallback(_selection.front());
				_boxStart.reset();
				invalidateRenderUnit();
			}
		}
		else if (p_event->button == spk::Mouse::Middle)
		{
			_panning = false;
		}
		else if (p_event->button == spk::Mouse::Right && _linkSource.has_value())
		{
			const std::optional<std::string> target = hitTest(local);
			if (target.has_value() && *target != *_linkSource)
			{
				if (_linkCallback) _linkCallback(*_linkSource, *target);
			}
			else if (target == _linkSource && _deleteCallback)
			{
				_deleteCallback(*_linkSource);
			}
			_linkSource.reset();
		}
	}

	void GraphCanvas::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		if (_panning)
		{
			_pan += spk::Vector2{static_cast<float>(p_event->delta.x), static_cast<float>(p_event->delta.y)} / _zoom;
			_refreshLabelGeometry();
			invalidateRenderUnit();
			p_event.consume();
			return;
		}
		if (_draggedNode.has_value())
		{
			Node *node = _findNode(*_draggedNode);
			if (node != nullptr)
			{
				node->position += spk::Vector2{
					static_cast<float>(p_event->delta.x) / (_zoom * UnitPixels),
					static_cast<float>(p_event->delta.y) / (_zoom * UnitPixels)};
				if (_moveCallback) _moveCallback(node->id, node->position);
				_refreshLabelGeometry();
				invalidateRenderUnit();
			}
			p_event.consume();
		}
	}

	void GraphCanvas::frameAll()
	{
		if (_nodes.empty()) { _pan = {}; _zoom = 1.0f; return; }
		float minX = _nodes.front().position.x, maxX = minX;
		float minY = _nodes.front().position.y, maxY = minY;
		for (const Node &node : _nodes)
		{
			minX = std::min(minX, node.position.x); maxX = std::max(maxX, node.position.x);
			minY = std::min(minY, node.position.y); maxY = std::max(maxY, node.position.y);
		}
		const float width = std::max(1.0f, maxX - minX + 1.5f) * UnitPixels;
		const float height = std::max(1.0f, maxY - minY + 1.0f) * UnitPixels;
		_zoom = std::clamp(std::min(
			static_cast<float>(std::max(1u, geometry().width())) / width,
			static_cast<float>(std::max(1u, geometry().height())) / height) * 0.85f, 0.25f, 2.0f);
		_pan = {-(minX + maxX) * UnitPixels * 0.5f, -(minY + maxY) * UnitPixels * 0.5f};
		_refreshLabelGeometry();
		invalidateRenderUnit();
	}

	void GraphCanvas::onSelection(std::function<void(const std::string &)> p_callback) { _selectionCallback = std::move(p_callback); }
	void GraphCanvas::onNodeMoved(std::function<void(const std::string &, const spk::Vector2 &)> p_callback) { _moveCallback = std::move(p_callback); }
	void GraphCanvas::onLinkToggled(std::function<void(const std::string &, const std::string &)> p_callback) { _linkCallback = std::move(p_callback); }
	void GraphCanvas::onNodeAdded(std::function<void(const spk::Vector2 &)> p_callback) { _addCallback = std::move(p_callback); }
	void GraphCanvas::onNodeDeleted(std::function<void(const std::string &)> p_callback) { _deleteCallback = std::move(p_callback); }
	const std::vector<GraphCanvas::Node> &GraphCanvas::nodes() const noexcept { return _nodes; }
	const std::vector<GraphCanvas::Edge> &GraphCanvas::edges() const noexcept { return _edges; }
	const std::vector<std::string> &GraphCanvas::selection() const noexcept { return _selection; }
	float GraphCanvas::zoom() const noexcept { return _zoom; }
	const spk::Vector2 &GraphCanvas::pan() const noexcept { return _pan; }
}
