# System of Recurrence Equations - UML & Architectural Analysis

The **System of Recurrence Equations** functionality in the `domain_flow` repository represents computations as recurrence relations over multi-dimensional index spaces (often referred to as Systems of Uniform or Affine Recurrence Equations: SURE or SARE).

This document identifies the core design abstractions, defines their responsibilities, details their relationships, and provides a **UML Class Diagram** using Mermaid.

---

## Core Components & Responsibilities

The System of Recurrence Equations module is comprised of the following key classes and structures:

### 1. [AffineMap](../include/dfa/affine_map.hpp#L33-L215)
A class template (`template<typename Scalar = int>`) representing an affine transformation between recurrence variables. An affine map of the form $f(x) = Ax + b$ maps coordinates in one iteration space to another.
* **Fields**:
  * `coefficients`: `MatrixX<Scalar>` - The linear transformation matrix ($A$).
  * `constants`: `VectorX<Scalar>` - The translation vector ($b$).
  * `inputDimension`: `size_t` - Dimension of the source coordinate space.
  * `outputDimension`: `size_t` - Dimension of the target coordinate space.
* **Key Operations**:
  * `operator*`: Composes two affine maps. If $f(x) = Ax + b$ and $g(x) = Cx + d$, then $(g \circ f)(x) = CAx + (Cb + d)$.
  * `apply`: Applies the affine transformation to a given input point: $Ax + b$.
  * `identity`: Static factory creating a square identity map.
  * `translation`: Static factory creating a translation-only affine map.
  * `addCoefficient` / `addConstant`: Fluent builder helpers.

### 2. [RecurrenceVariable](../include/dfa/recurrence_var.hpp#L12-L135)
Represents a computational variable inside a recurrence equation system.
* **Fields**:
  * `name`: `std::string` - Unique identifier of the variable.
  * `dimension`: `int` - The dimension of the variable's index space.
  * `dependencies`: `std::vector<std::pair<RecurrenceVariable*, AffineMap<int>>>` - Outgoing dependency edges mapping this variable to others via specific affine transformations.
  * Tarjan SCC state fields: `index`, `lowlink`, `onStack`.
* **Key Operations**:
  * `dependsOn`: Fluent method to append a dependency on another variable under a specific [AffineMap](../include/dfa/affine_map.hpp#L33-L215).
  * `dependsOnAll`: Batch dependency helper.
  * `withDimension`: Validates and fluently sets the dimension of the variable's index space.
  * `removeDependency` / `clearDependencies`: Dependency removal and cleanup.

### 3. [DependencyGraph](../include/dfa/dependency_graph.hpp#L47-L813)
The reduced dependency graph representing variables and their recurrence relationships. It coordinates analysis algorithms (SCC detection, cycle checks, topological sorting, and serialization/visualizations).
* **Fields**:
  * `variables`: `std::vector<std::unique_ptr<RecurrenceVariable>>` - Node ownership list.
  * `variableMap`: `std::map<std::string, RecurrenceVariable*>` - Quick name lookup table.
* **Key Operations**:
  * `createVariable`: Factory method to add a variable to the graph.
  * `findVariable` / `removeVariable`: Management operations.
  * `getStronglyConnectedComponents`: Implements Tarjan's SCC algorithm to group variables into mutually dependent components.
  * `isStronglyConnected`: Checks if the entire graph forms a single SCC.
  * `getCondensationGraph`: Condenses the SCCs into a Directed Acyclic Graph (DAG).
  * `getExecutionOrder`: Performs a topological sort on the condensation graph using Kahn's algorithm, establishing the execution sequence of SCCs.
  * `analyzeSCC` / `analyzeAllSCCs`: Analyzes structural properties of SCCs (elementary check, cycle detection, average degree).
  * `generateVisualization`: Serializes the graph structure into various formats (DOT, Mermaid, JSON, ASCII, HTML).

### 4. [DependencyGraph::Builder](../include/dfa/dependency_graph.hpp#L146-L259)
A nested helper class providing a fluent builder pattern API to safely construct [DependencyGraph](../include/dfa/dependency_graph.hpp#L47-L813) instances.
* **Fields**:
  * `graph`: `std::unique_ptr<DependencyGraph>`
  * `definedVariables`: `std::unordered_set<std::string>`
* **Key Operations**:
  * `variable`: Adds a single variable with validation checks (e.g. name format, dimensions).
  * `edge`: Connects two variables via an [AffineMap](../include/dfa/affine_map.hpp#L33-L215) dependency.
  * `variables` / `edges`: Batch declaration helpers.
  * `build`: Validates final structural constraints and returns the unique ownership pointer to the built graph.

### 5. [SCCProperties](../include/dfa/dependency_graph.hpp#L23-L35)
A supporting data structure holding properties of a strongly connected component of variables.
* **Fields**:
  * `size`: `size_t`
  * `hasSelfLoops`: `bool`
  * `isElementary`: `bool` (whether all variables in the SCC share the same dimension)
  * `maxDimension`: `int`
  * `cycles`: `std::vector<AffineMap<int>>` - A list of representative dependency cycles formed by composing maps along loops.
  * `averageDependencyDegree`: `double`

### 6. [VisualizationFormat](../include/dfa/dependency_graph.hpp#L38-L44)
An enumeration denoting output formats: `DOT`, `MERMAID`, `JSON`, `ASCII`, and `HTML`.

---

## UML Class Diagram

This Mermaid class diagram describes the static structure of the System of Recurrence Equations:

```mermaid
classDiagram
    direction TB
    
    class AffineMap~Scalar~ {
        -coefficients : MatrixX~Scalar~
        -constants : VectorX~Scalar~
        -inputDimension : size_t
        -outputDimension : size_t
        +AffineMap(coeffs, consts)
        +AffineMap(rows, cols, value)
        +operator*(other : AffineMap) AffineMap
        +apply(point : VectorX~Scalar~) VectorX~Scalar~
        +addCoefficient(coeff : Scalar, row : int, col : int) AffineMap&
        +addConstant(constant : Scalar, index : int) AffineMap&
        +getInputDimension() int
        +getOutputDimension() int
        +getCoefficients() MatrixX~Scalar~&
        +getConstants() VectorX~Scalar~&
        +identity(dimension : int) static AffineMap
        +translation(translation : vector~int~) static AffineMap
    }
    
    class RecurrenceVariable {
        -name : string
        -dimension : int
        -dependencies : vector~pair~RecurrenceVariable*, AffineMap~int~~~~
        -index : int
        -lowlink : int
        -onStack : bool
        +RecurrenceVariable(name : string, dim : int)
        +dependsOn(var : RecurrenceVariable*, map : AffineMap~int~&) RecurrenceVariable&
        +dependsOnAll(deps : vector~pair~RecurrenceVariable*, AffineMap~int~~~~) RecurrenceVariable&
        +withDimension(dim : int) RecurrenceVariable&
        +removeDependency(var : RecurrenceVariable*) RecurrenceVariable&
        +clearDependencies() RecurrenceVariable&
        +getName() string&
        +getDimension() int
        +getDependencies() vector~pair~RecurrenceVariable*, AffineMap~int~~~~&
        -isValidAffineMap(map, target) bool
    }
    
    class DependencyGraph {
        -variables : vector~unique_ptr~RecurrenceVariable~~
        -variableMap : map~string, RecurrenceVariable*~
        +createVariable(name : string, dimension : int) RecurrenceVariable&
        +findVariable(name : string) RecurrenceVariable*
        +removeVariable(name : string) bool
        +getVariableNames() vector~string~
        +isStronglyConnected() bool
        +getStronglyConnectedComponents() vector~vector~RecurrenceVariable*~~
        +hasUniformDependencies() bool
        +analyzeSCC(scc : vector~RecurrenceVariable*~) SCCProperties
        +analyzeAllSCCs() vector~SCCProperties~
        +getCondensationGraph() vector~pair~int, int~~
        +getExecutionOrder() vector~int~
        +generateVisualization(format : VisualizationFormat) string
    }
    
    class Builder {
        -graph : unique_ptr~DependencyGraph~
        -definedVariables : unordered_set~string~
        +Builder()
        +variable(name : string, dimension : int) Builder&
        +edge(from : string, to : string, map : AffineMap~int~) Builder&
        +variables(vars : vector~pair~string, int~~) Builder&
        +edges(edges : vector~tuple~string, string, AffineMap~int~~~) Builder&
        +build() unique_ptr~DependencyGraph~
    }
    
    class SCCProperties {
        +size : size_t
        +hasSelfLoops : bool
        +isElementary : bool
        +maxDimension : int
        +cycles : vector~AffineMap~int~~
        +averageDependencyDegree : double
    }
    
    class VisualizationFormat {
        <<enumeration>>
        DOT
        MERMAID
        JSON
        ASCII
        HTML
    }
    
    RecurrenceVariable "1" *-- "*" AffineMap~int~ : contains dependency mapping
    RecurrenceVariable "*" -- "*" RecurrenceVariable : depends on
    DependencyGraph "1" *-- "*" RecurrenceVariable : owns
    DependencyGraph ..> SCCProperties : computes
    DependencyGraph ..> VisualizationFormat : uses
    DependencyGraph +-- Builder : nested helper
```

---

## Key Dependency Analysis Flow

> [!NOTE]
> During graph compilation/validation:
> 1. Nodes (Variables) are mapped to multi-dimensional coordinate spaces using `withDimension`.
> 2. Directed Edges (Dependencies) carry the corresponding `AffineMap<int>` that specifies coordinates mapping from target to source.
> 3. Tarjan's algorithm groups dependencies into SCCs.
> 4. Cycle detection evaluates loop dependencies by composing their `AffineMap`s. If the composed map is uniform (e.g. translation-only with a negative offset/shift), it can be resolved statically.
