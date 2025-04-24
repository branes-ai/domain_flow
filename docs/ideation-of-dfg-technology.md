# Domain Flow Graph construction, refinement, and scheduling concepts

We have a domain flow graph data structure in C++ that contains operator types in its nodes. We need to expand that representation to contain information about the index space defined by the domain of computation constraints, and we need to derive the possible set of valid schedules for the parallel algorithm representing each operator, described as a domain flow program, which simplifies to a System of Uniform Recurrence Equations, or SURE. We would like to create a test that reads a domain flow graph, embellishes the operators with the SURE definition, and generate the scheduling vector feasibility set. How shall we test this?

Let’s break it down and design a testing strategy for your C++ implementation. The goal is to read a domain flow graph, augment its operator nodes with SURE definitions based on their index space (domain of computation constraints), and compute the feasible scheduling vectors. Here’s a step-by-step approach to create and test this functionality, with a focus on ensuring correctness and robustness.

---

### 1. Understand the Components
Before designing the test, let’s clarify the key components:

- **Domain Flow Graph (DFG)**: A directed graph where nodes represent operators (computations) and edges represent data dependencies. Each operator has an associated index space, defined by computation constraints (e.g., loop bounds like `i in [0, N), j in [0, M)`).
- **Index Space**: The multidimensional space of loop indices (e.g., `{i, j}`) over which an operator computes, constrained by inequalities (e.g., `0 ≤ i < N, 0 ≤ j < M`).
- **SURE (System of Uniform Recurrence Equations)**: A mathematical representation of the computation, where each operator’s computation at index point `p` (e.g., `{i, j}`) depends on values at uniformly shifted index points (e.g., `{i-1, j}, {i, j-1}`). For each operator, the SURE defines the recurrence pattern.
- **Scheduling Vectors**: A schedule assigns a time (scalar or vector) to each index point in the index space, ensuring dependencies are respected (i.e., if `p` depends on `q`, then `schedule(p) > schedule(q)`). For parallel algorithms, the schedule is often linear (e.g., `s(p) = s * p`, where `s` is a scheduling vector), and you need the set of all feasible `s` that satisfy dependency constraints.

The task is to:
1. Augment each operator node in the DFG with its SURE definition, derived from the index space and dependencies.
2. Compute the set of feasible scheduling vectors for the DFG, ensuring all dependencies are satisfied.
3. Test this functionality thoroughly.

---

### 2. Design the Implementation
To guide the testing strategy, let’s outline a high-level design of the implementation. This will help identify what to test.

#### Data Structures
- **Index Space**: Represent as a polyhedral set (e.g., using a library like ISL or PPL in C++). For example, `{[i, j] : 0 ≤ i < N, 0 ≤ j < M}`.
- **Operator Node**:
  - Original: Contains operator type (e.g., addition, convolution) and edges (dependencies).
  - Augmented: Add an index space (polyhedral set) and SURE definition (list of uniform dependencies, e.g., `{[i, j] → [i-1, j], [i, j-1]}`).
- **DFG**: A graph (e.g., `std::vector<Node>` for nodes, adjacency list for edges).
- **Scheduling Vectors**: Represent as a polyhedral cone of valid vectors `s`, where for each dependency `p → q`, the schedule satisfies `s * (p - q) ≥ 1`.

#### Algorithms
1. **Read DFG**: Parse the input DFG (e.g., from a file or in-memory structure) containing operator types and dependencies.
2. **Augment Operators with SURE**:
   - For each operator, derive its index space from constraints (e.g., provided as input or inferred from operator type).
   - Define the SURE by specifying uniform dependencies based on the operator type (e.g., a stencil operator might depend on neighboring indices).
3. **Compute Feasible Schedules**:
   - For each dependency edge (e.g., operator A at `p` depends on operator B at `q`), compute the dependency vector `d = p - q`.
   - Formulate scheduling constraints: For each dependency vector `d`, ensure `s * d ≥ 1`.
   - Use a polyhedral library (e.g., ISL) to compute the cone of valid `s` vectors.
4. **Output**: Store the SURE definitions in the DFG nodes and return the scheduling vector feasibility set (e.g., as a polyhedral cone).

#### Libraries
- Use a polyhedral library like **ISL (Integer Set Library)** or **PPL (Parma Polyhedral Library)** for index space and scheduling computations.
- Represent graphs using a library like **Boost.Graph** or a custom adjacency list.

---

### 3. Testing Strategy
To test this implementation, you need to verify:
- Correctness of SURE augmentation for each operator.
- Correctness of the scheduling vector feasibility set.
- Robustness against edge cases (e.g., cyclic graphs, empty index spaces, complex dependencies).

Here’s a detailed testing plan using a unit testing framework like **Google Test** in C++.

#### Test Setup
- **Framework**: Use Google Test for writing and running tests.
- **Mock DFGs**: Create small, well-defined DFGs as test inputs.
- **Polyhedral Library**: Assume ISL for polyhedral operations (adjust if using PPL or another library).
- **Test Data**:
  - Simple DFGs with known SUREs and schedules.
  - Complex DFGs with multiple operators and dependencies.
  - Edge cases (e.g., cyclic graphs, single-node graphs).

#### Test Cases
1. **Test SURE Augmentation**
   - **Goal**: Verify that each operator’s index space and SURE are correctly derived.
   - **Test Cases**:
     - **Single Operator (No Dependencies)**:
       - Input: A single node with operator type “matrix multiply” and index space `{[i, j] : 0 ≤ i < N, 0 ≤ j < M}`.
       - Expected: SURE with no dependencies (since no edges), index space correctly stored.
       - Example: For `C[i,j] = A[i,k] * B[k,j]`, the index space is `{[i,j] : 0 ≤ i,j < N}`, and SURE has no uniform dependencies (since it’s not a recurrence).
     - **Stencil Operator**:
       - Input: A node with a 2D stencil operator (e.g., 5-point stencil) and index space `{[i, j] : 1 ≤ i, j < N-1}`.
       - Expected: SURE with dependencies `{[i,j] → [i-1,j], [i+1,j], [i,j-1], [i,j+1]}`.
       - Example: For `U[i,j] = U[i-1,j] + U[i+1,j] + U[i,j-1] + U[i,j+1]`, verify the dependency vectors.
     - **Multiple Operators**:
       - Input: A DFG with two operators (e.g., A feeds into B) with index spaces `{[i] : 0 ≤ i < N}` and `{[i] : 0 ≤ i < N}`.
       - Expected: Each operator has its SURE, and dependencies reflect the edge (e.g., B[i] depends on A[i]).
   - **Implementation**:
     ```cpp
     TEST(SUREAugmentation, SingleOperator) {
         DFG dfg = createSingleNodeDFG("matmul", "{[i,j] : 0 <= i,j < 10}");
         augmentWithSURE(&dfg);
         ASSERT_EQ(dfg.nodes[0].index_space, parseISLSet("{[i,j] : 0 <= i,j < 10}"));
         ASSERT_TRUE(dfg.nodes[0].sure.dependencies.empty());
     }
     TEST(SUREAugmentation, StencilOperator) {
         DFG dfg = createSingleNodeDFG("stencil", "{[i,j] : 1 <= i,j < 9}");
         augmentWithSURE(&dfg);
         auto expected_deps = parseISLMap("{[i,j] -> [i-1,j]; [i,j] -> [i+1,j]; ... }");
         ASSERT_EQ(dfg.nodes[0].sure.dependencies, expected_deps);
     }
     ```

2. **Test Scheduling Vector Feasibility**
   - **Goal**: Verify that the computed scheduling vectors satisfy all dependency constraints.
   - **Test Cases**:
     - **Single Dependency**:
       - Input: DFG with two nodes A and B, where B[i] depends on A[i-1]. Index spaces: `{[i] : 1 ≤ i < N}` for both.
       - Expected: Scheduling cone `{s : s ≥ 1}` (since dependency vector `d = [1]`, so `s * 1 ≥ 1`).
     - **2D Stencil**:
       - Input: A single node with a 2D stencil, dependencies `{[i,j] → [i-1,j], [i,j-1]}`.
       - Expected: Scheduling cone `{[s1, s2] : s1 ≥ 1, s2 ≥ 1}` (since `d1 = [1,0]`, `d2 = [0,1]`).
     - **Complex DFG**:
       - Input: DFG with multiple nodes and dependencies (e.g., a pipeline or diamond-shaped graph).
       - Expected: Compute the scheduling cone and verify it contains known valid schedules (e.g., test with Farkas’ lemma or sample vectors).
   - **Implementation**:
     ```cpp
     TEST(Scheduling, SingleDependency) {
         DFG dfg = createTwoNodeDFG(
             "{[i] : 1 <= i < 10}", // A
             "{[i] : 1 <= i < 10}", // B
             "{[i] -> [i-1]}" // B[i] depends on A[i-1]
         );
         auto cone = computeSchedulingCone(dfg);
         auto expected = parseISLCone("{[s] : s >= 1}");
         ASSERT_EQ(cone, expected);
     }
     TEST(Scheduling, Stencil2D) {
         DFG dfg = createSingleNodeDFG("stencil", "{[i,j] : 1 <= i,j < 10}");
         dfg.nodes[0].sure.dependencies = parseISLMap("{[i,j] -> [i-1,j]; [i,j] -> [i,j-1]}");
         auto cone = computeSchedulingCone(dfg);
         auto expected = parseISLCone("{[s1,s2] : s1 >= 1, s2 >= 1}");
         ASSERT_EQ(cone, expected);
     }
     ```

3. **Test Edge Cases**
   - **Empty DFG**:
     - Input: DFG with no nodes.
     - Expected: Empty scheduling cone or default behavior (e.g., `{}`).
   - **Cyclic DFG**:
     - Input: DFG with a cycle (e.g., A → B → C → A).
     - Expected: Detect cycle and return empty scheduling cone (no valid linear schedule exists).
   - **Degenerate Index Space**:
     - Input: Operator with empty index space (e.g., `{[i] : i < 0}`).
     - Expected: Handle gracefully (e.g., skip or flag as invalid).
   - **Non-Uniform Dependencies**:
     - Input: Operator with non-uniform dependencies (e.g., `A[i] → B[i^2]`).
     - Expected: Flag as invalid for SURE (since SURE requires uniform dependencies).
   - **Implementation**:
     ```cpp
     TEST(EdgeCases, CyclicDFG) {
         DFG dfg = createCyclicDFG(3); // A -> B -> C -> A
         auto cone = computeSchedulingCone(dfg);
         ASSERT_TRUE(cone.is_empty());
     }
     TEST(EdgeCases, EmptyIndexSpace) {
         DFG dfg = createSingleNodeDFG("op", "{[i] : i < 0}");
         augmentWithSURE(&dfg);
         ASSERT_TRUE(dfg.nodes[0].index_space.is_empty());
     }
     ```

4. **Integration Test**
   - **Goal**: Test the entire pipeline (read DFG, augment SURE, compute schedules).
   - **Test Case**:
     - Input: A small DFG representing a parallel algorithm (e.g., a 2D heat equation solver).
     - Expected: Correct SUREs for all operators and a valid scheduling cone.
     - Example: For a 2D heat equation, expect dependencies like `{[i,j] → [i-1,j], [i,j-1]}` and a scheduling cone like `{[s1,s2] : s1 ≥ 1, s2 ≥ 1}`.
   - **Implementation**:
     ```cpp
     TEST(Integration, HeatEquation) {
         DFG dfg = loadDFGFromFile("heat_equation.dfg");
         augmentWithSURE(&dfg);
         auto cone = computeSchedulingCone(dfg);
         ASSERT_FALSE(cone.is_empty());
         // Verify specific dependencies and cone constraints
         for (const auto& node : dfg.nodes) {
             ASSERT_FALSE(node.sure.dependencies.is_empty());
         }
     }
     ```

#### Test Infrastructure
- **Helper Functions**:
  - `createSingleNodeDFG(type, index_space)`: Create a DFG with one node.
  - `createTwoNodeDFG(index_space1, index_space2, dependency)`: Create a DFG with two nodes and a dependency.
  - `parseISLSet(str)`, `parseISLMap(str)`, `parseISLCone(str)`: Parse ISL strings for testing.
- **Input Files**: For integration tests, create text files defining DFGs (e.g., JSON or custom format) with operator types, index spaces, and edges.
- **Validation**:
  - Use ISL to check polyhedral set equality.
  - For scheduling cones, test membership of known valid/invalid vectors (e.g., `s = [1,1]` should be valid for a 2D stencil).
- **Mocking**: If parsing DFGs from files, mock the file input for unit tests to avoid filesystem dependencies.

#### Testing Tools
- **Google Test**: For writing and running tests.
- **ISL**: For polyhedral operations and validation.
- **Valgrind**: To check for memory leaks.
- **Coverage Tools**: Use `gcov` or similar to ensure all code paths are tested.

---

### 4. Example Test Implementation
Here’s a minimal example using Google Test and ISL (pseudo-code for brevity):

```cpp
#include <gtest/gtest.h>
#include <isl/set.h>
#include <isl/map.h>
#include "dfg.h"

class DFGTest : public ::testing::Test {
protected:
    isl_ctx* ctx = isl_ctx_alloc();
    ~DFGTest() { isl_ctx_free(ctx); }
};

TEST_F(DFGTest, StencilSURE) {
    DFG dfg;
    Node node;
    node.type = "stencil";
    node.index_space = isl_set_read_from_str(ctx, "{[i,j] : 1 <= i,j < 10}");
    dfg.nodes.push_back(node);
    
    augmentWithSURE(&dfg);
    
    auto expected_deps = isl_map_read_from_str(ctx, "{[i,j] -> [i-1,j]; [i,j] -> [i+1,j]; [i,j] -> [i,j-1]; [i,j] -> [i,j+1]}");
    ASSERT_TRUE(isl_map_is_equal(dfg.nodes[0].sure.dependencies, expected_deps));
    isl_map_free(expected_deps);
}

TEST_F(DFGTest, SchedulingCone) {
    DFG dfg;
    Node a, b;
    a.index_space = isl_set_read_from_str(ctx, "{[i] : 1 <= i < 10}");
    b.index_space = isl_set_read_from_str(ctx, "{[i] : 1 <= i < 10}");
    dfg.nodes.push_back(a);
    dfg.nodes.push_back(b);
    dfg.edges.emplace_back(0, 1, isl_map_read_from_str(ctx, "{[i] -> [i-1]}"));
    
    auto cone = computeSchedulingCone(dfg);
    auto expected = isl_set_read_from_str(ctx, "{[s] : s >= 1}");
    ASSERT_TRUE(isl_set_is_equal(cone, expected));
    isl_set_free(cone);
    isl_set_free(expected);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

### 5. Edge Cases and Robustness
- **Numerical Stability**: Ensure polyhedral operations handle large index spaces (e.g., `N = 10^6`).
- **Invalid Inputs**: Test malformed DFGs (e.g., missing index spaces, invalid operator types).
- **Performance**: For large DFGs, test that the implementation scales (e.g., use a timeout or benchmark).
- **Cyclic Graphs**: Detect cycles using DFS and ensure the scheduling algorithm handles them correctly.
- **Non-SURE Computations**: If an operator has non-uniform dependencies, ensure the system either rejects it or handles it gracefully.

---

### 6. Debugging Tips
- **Log Intermediate Results**: Print index spaces, SURE dependencies, and scheduling constraints during tests to diagnose failures.
- **Visualize DFGs**: Use a graph visualization tool (e.g., Graphviz) to inspect test DFGs.
- **Simplify Test Cases**: Start with 1D index spaces and single dependencies, then scale to 2D and complex graphs.
- **Check Polyhedral Library Errors**: Ensure ISL/PPL operations (e.g., set intersections, cone computations) don’t fail silently.

---

### 7. Next Steps
- **Implement the DFG Parser**: Define a format for DFG inputs (e.g., JSON with nodes, edges, index spaces).
- **Integrate ISL/PPL**: Set up the polyhedral library for index space and scheduling computations.
- **Write More Tests**: Cover additional operator types (e.g., reductions, broadcasts) and DFG topologies.
- **Profile Performance**: Test on large DFGs to identify bottlenecks.
- **Validate with Real Algorithms**: Use DFGs from real parallel algorithms (e.g., FFT, convolution) to ensure practical correctness.

---

This testing strategy should give you confidence in your implementation. If you have specific operator types, DFG formats, or constraints you’d like to focus on, let me know, and I can tailor the tests further! Would you like help with a specific test case, ISL integration, or sample DFG input format?