# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Fixed

- **Compiler warnings cleanup**: Eliminated all 6,720 compiler warnings with `-Wall -Wextra` flags
  - Added `default:` cases to switch statements in `domain_flow_node.hpp` and `domain_of_computation.hpp` (4,545 warnings)
  - Fixed unused variable warnings with `[[maybe_unused]]` attributes across multiple headers (1,274 warnings)
  - Fixed signed/unsigned comparison warnings by using `size_t` for loop indices (567 warnings)
  - Fixed unused parameter warnings with `[[maybe_unused]]` attributes (319 warnings)
  - Removed unused local typedef declarations from test files (11 warnings)
  - Added missing struct field initializers in `energy_estimator.hpp` and `energy_estimator.cpp` (4 warnings)

- **Bug fix in `domain_flow_edge.hpp`**: Constructor now properly initializes `srcSlot` and `dstSlot` member variables from parameters (previously ignored)
