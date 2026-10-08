# Changelog

## 0.2.0

- Add one configure/build/test entry point with explicit platform selection.
- Validate typed CMake definitions, configuration, compilers, source/SDK locations
  and cached build identity before fetching or configuring.
- Apply selected macOS architectures and Visual Studio generator platforms to
  the actual application and test projects.
- Add `--aria-prefix` to consume an installed SDK at the exact locked version,
  without fetching source or silently selecting a different system SDK.
