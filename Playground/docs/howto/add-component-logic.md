# How-to — Add a data Component + its ComponentLogic

The ECS recipe (see [01-architecture.md §3](../01-architecture.md); working example:
`spk::TextureMeshRenderer3D` + `spk::TextureMeshRenderLogic`).

## 1. The component — data only

```cpp
// Playground/srcs/components/blinker.hpp
#pragma once
#include "structures/game_engine/spk_component.hpp"

namespace pg
{
    class Blinker : public spk::Component
    {
    private:
        float _period = 1.0f;
        float _elapsed = 0.0f;
    public:
        [[nodiscard]] float period() const { return _period; }
        void setPeriod(float p_period) { _period = p_period; }
        [[nodiscard]] float elapsed() const { return _elapsed; }
        void setElapsed(float p_elapsed) { _elapsed = p_elapsed; }
    };
}
```

No update methods, no engine references. If every holder needs a `Transform3D`, don't store
it — resolve it via `entity()->component<spk::Transform3D>()` in the logic (cache if hot).

## 2. The logic — one system for all instances

```cpp
// Playground/srcs/logics/blink_logic.hpp
#pragma once
#include "components/blinker.hpp"
#include "structures/game_engine/spk_component_logic.hpp"

namespace pg
{
    class BlinkLogic : public spk::ComponentLogic<Blinker>
    {
    protected:
        void _parseComponentForUpdate(const spk::UpdateContext &p_tick, Blinker &p_component) override
        {
            p_component.setElapsed(p_component.elapsed() +
                static_cast<float>(p_tick.deltaDuration().nbSeconds));
            // read/write sibling components via p_component.entity()
        }
    };
}
```

Hook cheat-sheet (all optional overrides):
- update: `_onUpdateStarted(tick)` → `_parseComponentForUpdate(tick, c)` → `_executeUpdate(tick)`
- render: `_onRenderStarted(count)` → `_parseComponentForRender(c)` →
  `_executeRender(spk::RenderUnitBuilder&)` — emit RenderCommands **only** here
- input: `_parseComponentForKeyPressedEvent(e, c)`, `_parseComponentForMouseButtonPressedEvent`,
  `_parseComponentForMouseMovedEvent`, … (`e.isConsumed()` short-circuits; consume events you
  fully handle)

## 3. Register & attach

```cpp
// scene setup (GameSceneWidget::_buildScene or a mode):
engine.add<pg::BlinkLogic>();               // once per engine
auto &blinker = entity.addComponent<pg::Blinker>();   // per entity
engine.addEntity(&entity);                  // entity must be engine-registered
```

Ordering between logics: `logic.setPriority(n)` (`spk::PriorizableTrait`, lower runs
first). Project bands: input/mode 0–99, simulation 100–199, view-sync 200–299, render
300+.

## Pitfalls

- Components are heap-owned by their entity; pointers to them stay valid until removal.
- `component<T>()` is a dynamic_cast walk — fine at setup, cache in per-frame paths.
- A logic is engine-global: guard for components whose entities belong to inactive
  modes/state via component activation (`spk::ActivableTrait`) rather than logic flags.
- Test rule: logic that computes goes through a plain function the test can call; keep the
  ComponentLogic subclass a thin adapter ([add-tests.md](add-tests.md)).
