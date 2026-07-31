---
title: Reducing presentation-geometry helper cost with measured predicate ordering
date: 2026-07-31
category: performance-issues
module: frame presentation geometry
problem_type: performance_issue
component: tooling
symptoms:
  - The presentation-geometry microbenchmark measured a 2.322099 ns/scenario seven-run median before predicate reordering.
  - Bounded fallback validated the area and aspect before checking whether an already-fitting presentation needed fallback.
root_cause: logic_error
resolution_type: code_fix
severity: low
tags: [presentation-geometry, microbenchmark, predicate-ordering, rendering-performance, benchmark-validation]
---

# Reducing presentation-geometry helper cost with measured predicate ordering

## Problem

[Issue #2246](https://github.com/BlitterStudio/amiberry/issues/2246) reported that enabling a filter could replace the presentation geometry selected by integer scaling. [PR #2250](https://github.com/BlitterStudio/amiberry/pull/2250) makes the selected presentation rectangle authoritative through the filter path. The follow-on task was to reduce the cost of the resulting geometry helpers without changing sizing or positioning.

`amiberry_gfx_final_presentation_rect()` normalizes invalid presentation dimensions, optionally fits an oversized presentation into a valid bounded area, and centers the result (`src/osdep/amiberry_gfx_geometry.h:32`). Because this helper controls visible geometry, a performance change was acceptable only when the focused tests, expected rectangles, and result checksum stayed unchanged.

## Symptoms

The synthetic eight-case benchmark measured a baseline median of **2.322099 ns per scenario** over seven invocations of 100,000,000 iterations. The samples ranged from 2.312088 to 2.336175 ns, with a standard deviation of 0.007615 ns.

The checked-in benchmark emits one timed sample per invocation. It performs a fixed warm-up, times one `run_scenarios()` call, and emits JSON with the timing, gates, checksum, and scenario count (`tests/gfx_geometry_benchmark.cpp:83`). The optimization workflow supplied the seven-run repetition and median aggregation.

The benchmark rotates through exactly eight fixed presentation cases with `& 7` (`tests/gfx_geometry_benchmark.cpp:21`, `tests/gfx_geometry_benchmark.cpp:65`). Three cases enable bounded fallback: one already fits and benefits from the early-false oversize check, while two are oversized. Invalid bounded inputs are covered by focused tests rather than the timed mix. This is useful for stable commit-to-commit comparison, but it is not a production profile of Amiberry sessions.

## What Didn't Work

Three plausible changes were measured and reverted:

| Attempt | Median | Result |
|---------|--------|--------|
| Pass the 16-byte rectangle inputs by value | 2.355136 ns | Slower than the 2.322099 ns baseline and noisier. This run did not demonstrate a benefit over the inlined `const` references. |
| Combine coverage comparisons eagerly for branchless evaluation | 3.322754 ns | Much slower. It forced all edge calculations and widened arithmetic instead of retaining short-circuit evaluation. |
| Add a zero-origin coverage fast path | 2.369229 ns | Slower than the current best and too noisy for a strong causal claim. The result was consistent with extra dispatch offsetting the two avoided additions. |

All three variants still passed the output gates and retained checksum 4260610592. Correctness gates therefore prevented regressions, but measurement was still required to choose a performance change.

## Solution

Keep the change small: after `use_bounded_fallback`, test whether the presentation is oversized before validating the bounded area's width, height, and aspect (`src/osdep/amiberry_gfx_geometry.h:40`).

```cpp
if (use_bounded_fallback
	&& (width > available_area.w || height > available_area.h)
	&& available_area.w > 0 && available_area.h > 0
	&& fallback_aspect > 0.0f) {
	// Fit the oversized presentation into the bounded area.
}
```

For a bounded presentation that already fits, the oversize expression is false and short-circuits the remaining three validity comparisons. The benchmark includes this path as well as oversized fallback paths (`tests/gfx_geometry_benchmark.cpp:26`).

The reordered predicate reduced the seven-run median to **2.193232 ns per scenario**, an improvement of **0.128867 ns or 5.55%** for that sample set. The seven samples ranged from 2.180707 to 2.220078 ns, with a standard deviation of 0.013738 ns. Expected rectangles, the focused tests, and checksum 4260610592 remained unchanged.

A later single-run validation measured 2.281184 ns per scenario. It remained faster than the baseline median, but the difference from the accepted median shows environmental drift. Baseline and candidate runs were serial rather than interleaved, so the 5.55% result should not be treated as universally reproducible.

The benchmark entry point compiles and runs the focused regression test before building the optimized benchmark with warnings treated as errors (`tests/benchmark_gfx_geometry.sh:15`). Review also added focused cases proving that zero-width, negative-height, and non-positive-aspect bounded inputs keep the requested presentation size (`tests/gfx_geometry_test.cpp:193`).

## Why This Works

The conjunction's meaning is unchanged. Bounded fitting still requires bounded mode, an oversized presentation, positive available dimensions, and a positive fallback aspect (`src/osdep/amiberry_gfx_geometry.h:40`). These operands are side-effect-free comparisons, so reordering them changes only where evaluation can stop.

When every predicate passes, the same aspect-fit helper computes the bounded dimensions. Otherwise, the same normalized dimensions are centered (`src/osdep/amiberry_gfx_geometry.h:44`, `src/osdep/amiberry_gfx_geometry.h:48`). Reference rectangles are checked before timing, and each timed run emits a checksum for external comparison (`tests/gfx_geometry_benchmark.cpp:47`, `tests/gfx_geometry_benchmark.cpp:63`). The checksum is an aggregate regression signal for this fixed input set, not proof of equivalence for every possible rectangle.

## Prevention

- Preserve the fixed eight-case cycle when comparing commits. Changing the scenario mix changes the meaning of the primary metric.
- Treat the cycle as a synthetic microbenchmark, not as evidence of production workload frequency.
- Repeat the one-sample harness externally and compare medians. Retain the raw samples and spread with the optimization decision. When practical, alternate baseline and candidate runs under pinned CPU-frequency conditions to reduce drift.
- Keep focused tests and reference-output checks coupled to performance measurements, and retain the emitted checksum for external comparison (`tests/benchmark_gfx_geometry.sh:15`, `tests/gfx_geometry_benchmark.cpp:47`).
- Retain invalid-input tests whenever predicate order changes; they protect guards that may move later in a short-circuit expression (`tests/gfx_geometry_test.cpp:193`).
- Measure compiler-oriented ideas. By-value inputs, eager branchless evaluation, and common-case specialization all appeared plausible but regressed this workload.

## Related Issues

- [Issue #2246](https://github.com/BlitterStudio/amiberry/issues/2246) is the filter and overlay scaling report that motivated the presentation-geometry work.
- [PR #2250](https://github.com/BlitterStudio/amiberry/pull/2250) contains the functional fix. It was open and unmerged as of 2026-07-31.
- [Issue #1862](https://github.com/BlitterStudio/amiberry/issues/1862) provides earlier integer-scaling correctness context.
