#pragma once

#include <dfa/dfg.hpp>

// MLIR Shim to derive a Domain Flow Graph from an MLIR IR
#include <dfa/mlir/dialect/tosa.hpp>
#include <dfa/mlir/dialect/torch.hpp>
#include <dfa/mlir/dialect/stablehlo.hpp>
#include <dfa/mlir/processModule.hpp>
