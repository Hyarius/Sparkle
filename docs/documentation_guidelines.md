# Sparkle API documentation guidelines

Sparkle is intended to be an educational library, so public API comments should teach both the purpose of a type and the way a learner is expected to use it.

## Public class comment checklist

Every public class or struct should include:

- `@brief`: one sentence explaining the role of the type.
- A short paragraph explaining when a user should choose this type.
- A `@code{.cpp}` example showing the normal usage pattern.
- `@tparam` entries for template parameters, when applicable.
- `@see` references to nearby concepts or classes that help the reader continue learning.

## Example template

```cpp
/**
 * @brief Short purpose of the class.
 *
 * Explain what problem the class solves and the most important command or method
 * a beginner should call first.
 *
 * @code{.cpp}
 * spk::Example example;
 * example.doTheCommonThing();
 * @endcode
 *
 * @see spk::RelatedClass
 * @see spk::AnotherClass
 */
class Example;
```

## Tone

Prefer practical comments over implementation trivia. A reader should understand what the type is for, how to start using it, and which related class to read next.
