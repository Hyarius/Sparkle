# How-to — Author & load a new JSON definition type

Recipe for adding a definition domain (or a polymorphic subtype). Schemas live in
[02-data-model.md](../02-data-model.md) — update it first, then code.

## 1. New domain `things/*.json`

```cpp
// srcs/things/thing_definition.hpp — immutable after load
struct ThingDefinition
{
    std::string id;          // filename stem, set by the registry
    int power = 0;
    const OtherDef *other;   // cross-refs resolved at load (string id in JSON)
};

// srcs/things/thing_parser.cpp
ThingDefinition parseThing(pg::JsonReader &p_reader, const pg::Registries &p_registries)
{
    p_reader.forbidUnknown({"version", "power", "other"});   // strictness, D10
    ThingDefinition result;
    result.power = p_reader.require<int>("power");
    result.other = &p_registries.others.get(p_reader.require<std::string>("other"));
    return result;
}
```

Register in the load sequence (`core/registries.cpp`) **after** every domain it references
(load order list in [02-data-model.md §1](../02-data-model.md)); bump nothing — new domains
start at `"version": 1`.

## 2. New polymorphic subtype (effect, requirement, shape, …)

```cpp
// in the hierarchy's factory TU (e.g. abilities/effect_factory.cpp):
factory.registerType("myEffect", [](pg::JsonReader &p_reader, auto &p_registries) {
    p_reader.forbidUnknown({"type", "amount"});
    auto result = std::make_unique<MyEffect>();
    result->amount = p_reader.require<int>("amount");
    return result;
});
```

Then: add the row to the type table in [02-data-model.md](../02-data-model.md), a
`describe()` sentence if it's an Effect (HUD rules text), and a parse test.

## 3. Author a file

`Playground/resources/data/things/first-thing.json` — id = `first-thing`, must carry
`"version": 1`. The game aborts on load errors with `file:jsonPath` context — that's the
fast feedback loop; no separate validator needed.

## 4. Tests (always)

- Parse a valid sample (string literal in the test, not a resource file).
- One test per failure class: missing field, unknown field, bad enum, dangling reference,
  wrong version — assert the `JsonError` message names the field.
- Round-trip if the type is tool-edited (JsonWriter → parse → equality).
