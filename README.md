# dfa-dynamics

Domain Flow Architecture dynamics

# Introduction

The vision of the Domain Flow Architecture repo is to build tools that can read and analyze MLIR bytecode.
The analysis is tailored to finding efficient schedules and spatial reductions to execute DL graphs efficiently
on a variety of hardware configurations.


The basic architecture represents DL graphs as pure domain flow graphs.
A domain flow graph is represented by chains of operators, each with a domain of computation. 
The operator is defined by a System of Affine Recurrence Equations. 
The domain of computation is derived from the tensor operands and the operator.
The most common representation of DNN models in the different DNN frameworks 
uses function nodes with tensor operands. This contains all the information
to derive the domain of computation for the Domain Flow graph.

The MLIR linalg and affine dialects represent loop nests and memory views.
In contrast the dfa dialect represents operators and domain flows.
Operators are hypothesized to execute in multi-dimensional data paths,
and the goal of the analysis is to find spatial reductions that avoid
resource contention.

