# Review 021 — `includes/structures/math/spk_perlin.hpp`

- File: [includes/structures/math/spk_perlin.hpp](<../includes/structures/math/spk_perlin.hpp>)
- Done [ ]
- Remarks and requested changes:

Review, complete, and document `spk::Perlin` as a deterministic one-, two-, and three-dimensional fractal Perlin-noise generator.

The class must provide predictable parameter validation, safe coordinate handling, reproducible seed behavior, clearly defined output ranges, and efficient read-only sampling.

## Clarify the abstraction

The class currently combines two layers:

1. Single-octave gradient noise.
2. Multi-octave fractal accumulation controlled by `octaves`, `persistence`, `lacunarity`, and `frequency`.

Make this distinction explicit in the implementation and documentation.

The private methods:

```cpp
_noise1D(...)
_noise2D(...)
_noise3D(...)
```

should represent one octave of gradient noise.

The public methods:

```cpp
raw1D(...)
raw2D(...)
raw3D(...)
```

should apply the complete configured fractal accumulation.

Document this clearly. Do not leave callers to guess whether `raw2D()` is one octave or the configured sum of all octaves.

A more explicit internal naming scheme is encouraged:

```cpp
_singleOctave1D(...)
_singleOctave2D(...)
_singleOctave3D(...)

_fractal1D(...)
_fractal2D(...)
_fractal3D(...)
```

The exact private names may follow project conventions.

## Rename the interpolation concept

The selected enum does not change the interpolation algorithm itself. The lattice contributions are still combined using linear interpolation.

It changes the fade curve applied to the local coordinate before interpolation.

Rename:

```cpp
enum class Interpolation
```

to a more accurate name such as:

```cpp
enum class Fade
{
	Linear,
	SmoothStep,
	SmootherStep
};
```

Then rename the parameter accordingly:

```cpp
Fade fade = Fade::SmootherStep;
```

The curve definitions must be exactly documented:

```cpp
Linear:
	t

SmoothStep:
	3t² - 2t³

SmootherStep:
	6t⁵ - 15t⁴ + 10t³
```

The fade input is expected to be in `[0, 1]`.

Use stable polynomial forms and avoid unnecessary calls to general-purpose power functions.

## Default constructor

Remove the unnecessary `explicit` specifier from the default constructor:

```cpp
Perlin();
```

Implement it by delegating to the parameterized constructor:

```cpp
Perlin() :
	Perlin(Parameters{})
{
}
```

Keep the parameterized constructor explicit:

```cpp
explicit Perlin(const Parameters &p_parameters);
```

## Parameter types

Replace the signed octave count:

```cpp
int octaves = 3;
```

with an explicit count type when practical:

```cpp
std::size_t octaves = 3;
```

Use the same type consistently in validation and octave loops.

Do not allow a negative octave value to be converted silently into a very large unsigned count.

## Adding a constructor with JSON::Value

Create a new constructor, a method toJSON and fromJSON inside perlin, to save/load perlin from json data, using sparkle JSON classes.

## Seed type and interpretation

Use the same canonical seed type as the shared deterministic generator.

Prefer:

```cpp
std::uint64_t seed = 0;
```

because Sparkle's master seeds and derived seeds are 64-bit values.

This prevents truncation when a seed is obtained through:

```cpp
spk::deterministic::deriveSeed(...)
```

The Perlin constructor and `reseed()` method should therefore use:

```cpp
explicit Perlin(const Parameters &p_parameters);

void reseed(std::uint64_t p_seed);
```

Permutation generation should conceptually begin with:

```cpp
spk::deterministic::Generator64 generator(p_seed);
```

The seed is not itself a random sample. It initializes the deterministic generator that constructs the permutation table.

The same seed, deterministic-generator version, and Perlin algorithm version must always produce the same permutation and the same noise field under the project's documented floating-point assumptions.

Changing any of the following changes the procedural format:

* Seed width.
* Generator transition.
* Generator seed initialization.
* Bounded-integer mapping.
* Shuffle direction.
* Gradient selection.
* Permutation-table construction.

Treat these changes as explicit deterministic-format migrations when existing generated content depends on them.

If legacy Perlin output must remain reproducible, keep the old generator path under a versioned implementation rather than retaining a duplicate private `_splitMix64()` indefinitely.

## Parameter validation

Centralize validation in:

```cpp
static void _validateParameters(
	const Parameters &p_parameters);
```

Validate every parameter before constructing or mutating object state.

Required rules should be explicit.

At minimum:

```text
octaves     > 0
persistence is finite and >= 0
lacunarity  is finite and > 0
frequency   is finite and > 0
fade        contains a supported enumerator
```

Consider whether the intended fractal model should additionally require:

```text
persistence <= 1
lacunarity  >= 1
```

These are common defaults, but values outside those conventional ranges can still be mathematically meaningful.

Do not reject them arbitrarily. Permit all finite positive values and document the resulting behavior.

Throw `std::invalid_argument` for invalid parameters.

Do not silently clamp configuration values.

## Strong mutation guarantee

Both constructors and mutation methods must validate and prepare all new state before modifying the existing object.

For:

```cpp
void setParameters(const Parameters &p_parameters);
```

the operation must provide the strong exception guarantee.

A suitable sequence is:

1. Validate the supplied parameters.
2. Build a new permutation table if the seed changes.
3. Commit the parameters and table only after every potentially throwing operation succeeds.

Do not update `_parameters` first and leave the table inconsistent if rebuilding fails.

For:

```cpp
void reseed(std::uint64_t p_seed);
```

or the retained 32-bit equivalent:

1. Build the replacement permutation table.
2. Commit the new seed and table together.

When the new seed equals the current seed, the method may return without rebuilding.

When `setParameters()` changes only frequency, octave, persistence, lacunarity, or fade settings, do not rebuild the permutation table unnecessarily.

## Thread-safety contract - mutation removal

Sampling methods are read-only and should be safe to call concurrently on the same object as long as no thread mutates that object.

As runtime mutation is unnecessary, consider making the class immutable by removing mutation methods and constructing a new `Perlin` object for a new configuration.

## Avoid modulo bias in permutation generation

A shuffle expression such as:

```cpp
randomValue % bound
```

introduces modulo bias unless the random range is an exact multiple of the bound.

For a 256-element permutation the practical bias may be small, but the corrected implementation should use unbiased bounded sampling when introducing a new deterministic format.

Use rejection sampling or another precisely defined unbiased mapping.

## Permutation periodicity

A classic 256-entry permutation table makes the lattice hash repeat every 256 integer cells in each dimension.

Therefore, the base noise is periodic with period 256 lattice units. With the configured base frequency, the world-space period is related to:

```text
256 / frequency
```

and each octave introduces its own scaled repetition.

This can become visible in large generated maps.

Before retaining the current table-based design, inspect the intended sampling scale. Sparkle world generation may operate over maps substantially larger than 256 cells.

### Coordinate-hashed Perlin noise

Replace permutation lookup with a deterministic hash of:

```text
seed
integer x
integer y
integer z
```

to select gradients without a short 256-cell period.

This is preferable for effectively unbounded procedural worlds when visible repetition is undesirable.

The coordinate hash must use canonical fixed-width integer encoding and a stable deterministic algorithm.

Do not change from permutation-based noise to coordinate-hashed noise silently. The entire generated field will change.

## Coordinate safety

The current helper:

```cpp
static int _fastFloor(float p_value) noexcept;
```

is unsafe for finite values outside the representable range of `int`.

Converting a sufficiently large floating-point value to an integer can produce undefined or implementation-dependent behavior.

Octave multiplication can also push initially reasonable coordinates beyond the safe lattice range.

Use a fixed-width lattice coordinate type:

```cpp
using LatticeCoordinate = std::int64_t;
```

or another explicitly justified type.

Before conversion:

* Reject NaN and infinity.
* Verify that the scaled coordinate can be safely floored and represented.
* Account for access to the neighboring lattice coordinate.
* Avoid signed overflow when computing the upper corner.

Do not compute:

```cpp
x1 = x0 + 1;
```

when `x0` may already be the maximum representable integer.

A safe design can reduce or hash the lower lattice coordinate first and derive the neighboring wrapped index without overflowing the signed coordinate.

Do not rely on host-specific signed representation or signed overflow.

## Input validation

The public sampling methods must reject non-finite coordinates:

```cpp
NaN
positive infinity
negative infinity
```

Throw `std::invalid_argument`.

Also reject coordinates that become unsafe after applying the configured frequency and octave lacunarity.

Do not mark public sampling methods `noexcept` when they validate and may throw.

Private single-octave methods may remain `noexcept` only when their preconditions are guaranteed by the public layer.

## Frequency semantics

Document exactly how frequency is applied.

The expected fractal progression is:

```text
octaveFrequency = parameters.frequency
octaveAmplitude = 1

for each octave:
	sample single-octave noise at position * octaveFrequency
	accumulate sample * octaveAmplitude
	octaveFrequency *= lacunarity
	octaveAmplitude *= persistence
```

Do not alter the input coordinates cumulatively in a way that introduces avoidable floating-point drift.

Use local frequency and amplitude variables.

Check multiplication for non-finite results before sampling the next octave.

## Fractal amplitude normalization

Define that `raw1D()`, `raw2D()`, and `raw3D()` return the weighted sum divided by the total amplitude.

A conventional accumulation is:

```cpp
float sum = 0.0f;
float amplitude = 1.0f;
float amplitudeSum = 0.0f;

for (...)
{
	sum += octaveNoise * amplitude;
	amplitudeSum += amplitude;
	amplitude *= persistence;
}

return sum / amplitudeSum;
```

This is only sufficient if the single-octave noise itself has a documented range.

Do not divide by zero. Parameter validation must ensure a valid positive amplitude sum.

When persistence is zero, only the first octave contributes even if the configured octave count is greater than one.

## Define the single-octave range

The magnitude of gradient noise depends on:

* The selected gradient vectors.
* Whether those vectors are normalized.
* Dimensionality.
* The fade curve.
* Any final scaling constant.

Do not assume without proof that every private noise function naturally returns exactly within `[-1, 1]`.

Define the exact gradient sets used by each dimension.

Recommended behavior:

### One dimension

Use gradients:

```text
-1
+1
```

### Two dimensions

Use a documented set of axis and/or diagonal gradients.

When diagonal vectors are included, normalize them or apply a documented scale so diagonal gradients do not have greater magnitude than axis gradients.

### Three dimensions

Use a documented, symmetric gradient set such as the classic 12 edge directions, with a documented normalization policy.

Add dimension-specific output scaling only when it is mathematically justified and covered by tests.

The three dimensional implementations do not need to produce identical distributions, but each must have clearly defined and stable behavior.

## Meaning of `raw*()`

Document `raw1D()`, `raw2D()`, and `raw3D()` precisely.

A recommended contract is:

```text
Returns the configured normalized fractal-noise value.
Expected mathematical range: approximately [-1, 1].
No mapping to a caller-provided range is performed.
```

If the implementation can guarantee the strict range, say so and test it.

If the range is only approximate, do not claim strict bounds.

Consider renaming these methods to:

```cpp
noise1D(...)
noise2D(...)
noise3D(...)
```

because the result is already configured, interpolated, and octave-accumulated rather than truly raw.

Keep the current names when they are already established in the public API, but improve their documentation.

## Meaning of `sample*()`

The range-mapped methods must have a stronger contract:

```cpp
sample2D(x, y, min, max)
```

returns a value within:

```text
[min, max]
```

for every valid finite input.

Validate:

```text
min is finite
max is finite
min <= max
```

Allow:

```cpp
min == max
```

and return that exact value.

Throw `std::invalid_argument` when the range is invalid.

If `raw*()` can slightly exceed `[-1, 1]`, clamp the normalized mapping input before converting to the target range:

```cpp
normalized = std::clamp(
	(raw + 1.0f) * 0.5f,
	0.0f,
	1.0f);
```

Then use:

```cpp
std::lerp(p_min, p_max, normalized)
```

or an equivalent stable expression.

Do not allow `sample*()` to escape its documented range because of floating-point overshoot.

## Avoid conflating clamping and algorithm correctness

Clamping in `sample*()` is acceptable to guarantee the requested output range.

Do not use that clamp to conceal a badly scaled single-octave implementation.

Tests must still inspect raw values and validate the intended distribution and bounds independently.

## Gradient hashing

The hash methods:

```cpp
_hash1D(...)
_hash2D(...)
_hash3D(...)
```

must have a documented lookup order.

For permutation-based classic Perlin noise, use one consistent nesting convention and test it with golden values.

Avoid unsafe signed indexing.

Every table index must be converted to `std::size_t` before indexing.

Keep all accesses within the duplicated permutation table.

If coordinate hashing replaces the permutation table, remove these misleading permutation-specific hash helpers and use dimension-specific coordinate hash functions.

## Arithmetic precision

Decide deliberately whether internal accumulation uses `float` or `double`.

The public result may remain `float`.

Using `double` for:

* Scaled coordinates.
* Frequency progression.
* Amplitude accumulation.
* Weighted sum.

can reduce overflow and accumulated precision loss, but may change existing deterministic outputs.

Do not change precision silently when bit-for-bit compatibility matters.

Document that cross-platform deterministic seed and permutation generation does not automatically guarantee bit-identical floating-point output under every compiler option and architecture.

Bit-identical noise values may require:

* Consistent IEEE-754 support.
* Consistent contraction or FMA policy.
* No fast-math transformations.
* A defined intermediate precision policy.

State the actual guarantee the project intends to provide.

## Copy and move behavior

The class contains only parameters and local permutation state, so ordinary value semantics are appropriate.

Explicitly default copy and move operations when that improves clarity:

```cpp
Perlin(const Perlin &) = default;
Perlin &operator=(const Perlin &) = default;
Perlin(Perlin &&) noexcept = default;
Perlin &operator=(Perlin &&) noexcept = default;
```

This is optional when implicit behavior is already clear and consistent with project style.

Copies must produce identical samples.

## Parameter access

Keep:

```cpp
[[nodiscard]]
const Parameters &parameters() const noexcept;
```

when returning a reference matches project conventions.

The returned reference must not permit mutation.

Document that the reference remains valid for the object's lifetime but reflects later calls to `setParameters()`.

Returning `Parameters` by value is also acceptable because the structure is small and avoids exposing internal storage lifetime. Choose one style consistently with the rest of Sparkle.

## Suggested public interface

After review, the public API may approximately become:

```cpp
namespace spk
{
	class Perlin
	{
	public:
		enum class Fade
		{
			Linear,
			SmoothStep,
			SmootherStep
		};

		struct Parameters
		{
			std::uint64_t seed = 0;
			std::size_t octaves = 3;
			float persistence = 0.5f;
			float lacunarity = 2.0f;
			float frequency = 1.0f;
			Fade fade = Fade::SmootherStep;
		};

		Perlin();
		explicit Perlin(const Parameters &p_parameters);

		void reseed(std::uint64_t p_seed);
		void setParameters(const Parameters &p_parameters);

		[[nodiscard]]
		const Parameters &parameters() const noexcept;

		[[nodiscard]]
		float raw1D(float p_x) const;

		[[nodiscard]]
		float raw2D(float p_x, float p_y) const;

		[[nodiscard]]
		float raw3D(
			float p_x,
			float p_y,
			float p_z) const;

		[[nodiscard]]
		float sample1D(
			float p_x,
			float p_min = 0.0f,
			float p_max = 1.0f) const;

		[[nodiscard]]
		float sample2D(
			float p_x,
			float p_y,
			float p_min = 0.0f,
			float p_max = 1.0f) const;

		[[nodiscard]]
		float sample3D(
			float p_x,
			float p_y,
			float p_z,
			float p_min = 0.0f,
			float p_max = 1.0f) const;
	};
}
```

This interface is illustrative.

Preserve established names and seed width when compatibility requires them, but implement and document the behavioral corrections described in this review.

## Optional vector overloads

Inspect Sparkle's vector types and call sites.

When procedural code commonly stores positions in vectors, consider convenience overloads such as:

```cpp
float raw2D(const Vector2Float &p_position) const;
float raw3D(const Vector3Float &p_position) const;
```

Do not replace the scalar overloads.

Do not add these overloads when they would introduce unnecessary dependencies or are not used naturally by the repository.

## Includes

The declaration should approximately require:

```cpp
#include <array>
#include <cstddef>
#include <cstdint>
```

The implementation may require:

```cpp
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
```

When the class uses the shared deterministic utility, include its narrow header.

Do not include random-engine headers when the implementation uses a fully specified local deterministic shuffle.

## Tests — construction and parameters

Add tests covering:

* Default construction.
* Default seed.
* Default octave count.
* Default persistence.
* Default lacunarity.
* Default frequency.
* Default fade curve.
* Construction with custom valid parameters.
* Rejection of zero octaves.
* Rejection of negative octaves when a signed type is retained.
* Rejection of NaN persistence.
* Rejection of infinite persistence.
* Rejection of negative persistence.
* Rejection of zero frequency.
* Rejection of negative frequency.
* Rejection of non-finite frequency.
* Rejection of zero lacunarity.
* Rejection of negative lacunarity.
* Rejection of non-finite lacunarity.
* Rejection of invalid fade-enum values.
* Acceptance or rejection of persistence above one according to the documented contract.
* Acceptance or rejection of lacunarity below one according to the documented contract.
* Equal parameter objects producing identical results.

## Tests — permutation generation

For a permutation-based implementation, add tests covering:

* Every base value from `0` to `255` appears exactly once.
* The second half duplicates the first half.
* The same seed produces the same permutation.
* Different representative seeds produce different permutations.
* Golden permutation prefixes for fixed seeds.
* Seed zero.
* Maximum supported seed.
* Deterministic output across supported compilers.
* Unbiased bounded-index generation separately when practical.

Do not expose the permutation table publicly solely for testing. Use a test friend, private test access convention, or verify through sufficient golden noise samples according to project practices.

## Tests — deterministic samples

Add hard-coded golden samples for every supported dimension.

Cover:

* Seed zero.
* A non-zero seed.
* Positive coordinates.
* Negative coordinates.
* Fractional coordinates.
* Zero coordinates.
* Multiple fade curves.
* Multiple octave configurations.
* Multiple frequencies.
* Multiple persistence values.
* Multiple lacunarity values.
* Reseeding.
* Copy construction.
* Copy assignment.

Do not compute expected results through a second copy of the implementation under test.

Record compatibility vectors before changing any established algorithm.

## Tests — lattice behavior

For classic gradient noise, add tests covering:

* Single-octave noise at integer lattice coordinates.
* Continuity immediately to either side of lattice boundaries.
* Negative lattice coordinates.
* Neighboring cells.
* Periodicity after 256 lattice cells when the permutation design is retained.
* Absence of the 256-cell repetition when coordinate hashing is selected.

Avoid asserting derivative smoothness for the `Linear` fade mode.

For `SmoothStep` and `SmootherStep`, test the expected endpoint behavior within a reasonable floating-point tolerance.

## Tests — octave behavior

Add tests proving:

* One octave matches the corresponding single-octave result after documented scaling.
* Persistence zero makes later octaves contribute nothing.
* Increasing octaves preserves the documented normalization policy.
* Frequency scales coordinates as documented.
* Lacunarity scales octave frequency as documented.
* Repeated sampling does not mutate the object.
* Extreme valid configurations either produce finite results or throw according to the documented overflow policy.

## Tests — mapped ranges

Add tests covering:

* Default `[0, 1]` mapping.
* Negative output ranges.
* Ranges crossing zero.
* Equal minimum and maximum.
* Rejection of `min > max`.
* Rejection of NaN bounds.
* Rejection of infinite bounds.
* Every returned mapped sample remains inside the requested range.
* Raw negative endpoint mapping to the minimum when directly testable.
* Raw positive endpoint mapping to the maximum when directly testable.
* Floating-point overshoot is clamped safely.

## Tests — invalid coordinates

Add tests covering:

* NaN coordinates.
* Positive infinity.
* Negative infinity.
* Very large finite coordinates.
* Very large negative coordinates.
* Coordinates that become non-finite after frequency scaling.
* Coordinates that exceed the selected lattice type after octave scaling.

Test all dimensions.

## Tests — concurrency

When the documented contract permits concurrent const sampling, add a deterministic test that samples the same immutable object from several threads.

Verify that:

* Every thread obtains the expected values.
* No state is mutated.
* Results match equivalent single-threaded sampling.

Do not concurrently call `reseed()` or `setParameters()` unless the class is deliberately redesigned to support that behavior.

## Performance considerations

Sampling may occur for millions of world positions.

Keep the hot path allocation-free.

Do not:

* Rebuild the permutation table during sampling.
* Allocate temporary containers.
* Use virtual dispatch.
* Acquire a mutex for immutable sampling.
* Revalidate unchanged configuration for every octave.
* Use expensive general-purpose power functions for fade curves.
* Convert coordinates to strings for hashing.

Precompute immutable configuration data when useful, such as:

* Amplitude normalization.
* Validated octave count.
* Dimension-independent constants.

Do not add caches keyed by floating-point coordinates without a demonstrated need.

Benchmark before introducing SIMD, lock-free caches, or other complex optimizations.

## Scope restrictions

Do not add unrelated noise families such as:

* Simplex noise.
* OpenSimplex noise.
* Worley noise.
* Value noise.
* Domain warping.
* Ridged multifractal noise.

Do not add four-dimensional noise unless an actual caller requires it.

Do not make the class depend on global mutable random state.

Do not use `std::hash` as a persistence or procedural-generation contract.

The objective is a precise and reusable Perlin-noise component with:

* Stable and explicit seed behavior.
* Safe finite coordinate handling.
* Clearly defined octave semantics.
* Documented output ranges.
* Efficient concurrent read-only sampling.
* No accidental short-period repetition unless that periodicity is deliberately accepted.
