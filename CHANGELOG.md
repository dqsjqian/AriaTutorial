# Changelog

## 0.2.0

- Select the published Aria 3.2.0 and Mira 1.1.1 dependencies.
- Add `--all-demos` to build and test all 21 examples, including Qt6 and HTTP,
  through one entry point. Exercise both optional chapters in Linux CI.

- Add one configure/build/test entry point with explicit platform selection.
- Validate typed CMake definitions, configuration, compilers, source/SDK locations
  and cached build identity before fetching or configuring.
- Apply selected macOS architectures and Visual Studio generator platforms to
  the actual application and test projects.
- Add `--aria-prefix` to consume an installed SDK at the exact locked version,
  without fetching source or silently selecting a different system SDK.
