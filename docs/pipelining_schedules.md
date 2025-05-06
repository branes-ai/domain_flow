# Pipelining schedules

To address the challenge of modulating the schedules of dependent operators in a domain flow graph (DFG) based on the availability of results from supplying operators, we need a mechanism that accounts for the fine-grained dependencies and the structure of the index spaces. Specifically, you’re dealing with operators like matrix multiplication (matmul) followed by ReLU, where the linear schedules (dot products of scheduling vectors with index space points) for the supplying operator (e.g., matmul) determine when its outputs are available, impacting the feasible schedules for the dependent operator (e.g., ReLU). The schedules are derived from embedding the fine-grained computation graph of a System of Uniform Recurrence Equations (SURE) into a 3D space, and you’re seeking a generalized approach to propagate scheduling constraints between operators.

Below, I propose a generalized mechanism to modulate the schedules of dependent operators, leveraging polyhedral techniques and dependency analysis, tailored to your DFG and SURE framework. I’ll also incorporate insights from your previous questions (e.g., tensor shapes, matmul SUREs, and index spaces) to ensure relevance to your project. The suggestions aim to be both practical for implementation in C++ (using tools like ISL) and flexible for various operator types and dependency patterns.

---

### 1. Problem Recap and Key Concepts
Let’s formalize the problem to guide the solution:

- **Domain Flow Graph (DFG)**: A directed graph where nodes are operators (e.g., matmul, ReLU) with associated index spaces and SUREs, and edges represent data dependencies (e.g., matmul’s output feeds into ReLU’s input).
- **Index Space**: Each operator computes over a multidimensional index space (e.g., ${[b, i, j] : 0 ≤ b < batchSize, 0 ≤ i < m, 0 ≤ j < n}$ for a batched matmul).
- **SURE**: Defines the fine-grained computation as uniform recurrences (or reductions, like matmul). For example, a batched matmul’s SURE might be:
  $$\eqalign{
  C[b, i, j] = \sum_{k} A[b, i, k] \cdot B[b, k, j]
  }$$
  with dependencies from ${ A[b, i, k] }$ and ${ B[b, k, j] }$ to ${ C[b, i, j] }$.
- **Linear Schedules**: Each operator’s computation at index point ${ \mathbf{p} = [p_1, p_2, \dots] }$ is assigned a time via a scheduling vector \( \mathbf{s} \):
  \[
  \text{schedule}(\mathbf{p}) = \mathbf{s} \cdot \mathbf{p} = s_1 p_1 + s_2 p_2 + \dots
  \]
  The schedule must satisfy dependency constraints: if \( \mathbf{p} \) depends on \( \mathbf{q} \), then \( \mathbf{s} \cdot \mathbf{p} > \mathbf{s} \cdot \mathbf{q} \).
- **3D Embedding**: The fine-grained SURE computation graph is embedded into a 3D space (e.g., representing spatial or temporal dimensions), constraining the scheduling vectors to a 3D form (e.g., \( \mathbf{s} = [s_1, s_2, s_3] \)).
- **Challenge**: The matmul’s schedule determines the order in which output tensor elements (e.g., \( C[b, i, j] \)) are computed. The ReLU operator, which consumes \( C \), can only compute its output (e.g., \( \text{ReLU}(C[b, i, j]) \)) when \( C[b, i, j] \) is available. We need a mechanism to ensure the ReLU’s schedule respects the matmul’s output availability while remaining flexible and generalizable.

#### Example Scenario
- **Matmul Operator**:
  - Input tensors: \( A \) of shape `(batchSize, m, k)`, \( B \) of shape `(batchSize, k, n)`.
  - Output tensor: \( C \) of shape `(batchSize, m, n)`.
  - Index space: \( \{[b, i, j] : 0 \leq b < \text{batchSize}, 0 \leq i < m, 0 \leq j < n\} \).
  - Schedule: \( \mathbf{s_m} \cdot [b, i, j] \), where \( \mathbf{s_m} = [s_{m1}, s_{m2}, s_{m3}] \) (3D embedding).
  - Example: If \( \mathbf{s_m} = [1, 0, 0] \), computations are ordered by batch index \( b \), meaning all \( C[b, :, :] \) are computed before \( C[b+1, :, :] \).
- **ReLU Operator**:
  - Input tensor: \( C \) of shape `(batchSize, m, n)`.
  - Output tensor: \( D[b, i, j] = \max(0, C[b, i, j]) \).
  - Index space: Same as matmul’s output, \( \{[b, i, j] : 0 \leq b < \text{batchSize}, 0 \leq i < m, 0 \leq j < n\} \).
  - Schedule: \( \mathbf{s_r} \cdot [b, i, j] \), where \( \mathbf{s_r} = [s_{r1}, s_{r2}, s_{r3}] \).
  - Dependency: \( D[b, i, j] \) requires \( C[b, i, j] \), so:
    \[
    \mathbf{s_r} \cdot [b, i, j] > \mathbf{s_m} \cdot [b, i, j]
    \]
- **Issue**: The matmul’s schedule \( \mathbf{s_m} \) dictates when \( C[b, i, j] \) is available. For example, if \( \mathbf{s_m} = [0, 1, 0] \), computations are ordered by \( i \), so \( C[b, i, :] \) is available before \( C[b, i+1, :] \). The ReLU’s schedule \( \mathbf{s_r} \) must ensure \( D[b, i, j] \) is computed after \( C[b, i, j] \), but we want a general mechanism to propagate this constraint across arbitrary operators and schedules.

---

### 2. Generalized Mechanism for Schedule Modulation
To modulate the schedule of a dependent operator (e.g., ogni ReLU) based on the availability of results from a supplying operator (e.g., matmul), I propose a **dependency-driven scheduling framework** that propagates constraints through the DFG. The mechanism uses polyhedral analysis to model inter-operator dependencies and compute feasible schedules, accounting for the fine-grained SURE dependencies and the 3D embedding. Here’s the approach:

#### Step 1: Model Inter-Operator Dependencies
- **Dependency Edges**: For each edge in the DFG (e.g., matmul → ReLU), define the data dependency at the index space level.
  - Example: For matmul → ReLU, the dependency is:
    \[
    D[b, i, j] \text{ depends on } C[b, i, j]
    \]
    In polyhedral terms, this is a dependency map:
    \[
    \{ [b, i, j] \rightarrow [b, i, j] : 0 \leq b < \text{batchSize}, 0 \leq i < m, 0 \leq j < n \}
    \]
    This map indicates that each index point in ReLU’s index space depends on the corresponding point in matmul’s output.
- **General Form**: For any edge from operator \( O_1 \) (supplying) to \( O_2 \) (dependent), define a dependency map:
  \[
  M_{12} : D_1 \rightarrow D_2
  \]
  where \( D_1 \) is \( O_1 \)’s output index space, and \( D_2 \) is \( O_2 \)’s input index space. The map \( M_{12} \) specifies which output points of \( O_1 \) are needed for each input point of \( O_2 \).
  - For matmul → ReLU, \( M_{12} \) is an identity map (1:1 correspondence).
  - For other operators (e.g., convolution → ReLU), \( M_{12} \) might involve shifts (e.g., stencil dependencies).

- **Implementation**:
  - Store the dependency map in each DFG edge using ISL (Integer Set Library):
    ```cpp
    struct DFGEdge {
        int srcNodeId, dstNodeId;
        isl_map* dependencyMap; // e.g., "{[b,i,j] -> [b,i,j]}"
    };
    ```
  - Derive \( M_{12} \) from the SUREs of \( O_1 \) and \( O_2 \). For ReLU, the SURE is pointwise:
    \[
    D[b, i, j] = \max(0, C[b, i, j])
    \]
    For matmul, it’s a reduction (as discussed in your previous question), but the output \( C[b, i, j] \) is the final result at each index point.

#### Step 2: Propagate Scheduling Constraints
- **Intra-Operator Schedules**: For each operator, compute a linear schedule \( \mathbf{s} \cdot \mathbf{p} \) that satisfies its internal SURE dependencies (as in your current approach).
  - For matmul:
    - Dependencies include \( A[b, i, p] \rightarrow C[b, i, j] \), \( B[b, p, j] \rightarrow C[b, i, j] \).
    - Constraints: \( \mathbf{s_m} \cdot [b, i, j] > \mathbf{s_m} \cdot [b, i, p] \), etc.
  - For ReLU:
    - No internal dependencies (pointwise operation), so any \( \mathbf{s_r} \) is valid internally.
- **Inter-Operator Constraints**: For each dependency edge \( O_1 \rightarrow O_2 \), add scheduling constraints based on the dependency map \( M_{12} \).
  - For each pair \( (\mathbf{p_1}, \mathbf{p_2}) \in M_{12} \), where \( \mathbf{p_1} \in D_1 \) (output of \( O_1 \)) and \( \mathbf{p_2} \in D_2 \) (input of \( O_2 \)), ensure:
    \[
    \mathbf{s_2} \cdot \mathbf{p_2} > \mathbf{s_1} \cdot \mathbf{p_1}
    \]
    - For matmul → ReLU, since \( M_{12} = \{ [b, i, j] \rightarrow [b, i, j] \} \):
      \[
      \mathbf{s_r} \cdot [b, i, j] > \mathbf{s_m} \cdot [b, i, j]
      \]
      Simplifying for all \( [b, i, j] \):
      \[
      \mathbf{s_r} \cdot [b, i, j] \geq \mathbf{s_m} \cdot [b, i, j] + 1
      \]
      This ensures ReLU at \( [b, i, j] \) is scheduled after matmul produces \( C[b, i, j] \).

- **3D Embedding**: Since schedules are embedded in 3D space, \( \mathbf{s_1}, \mathbf{s_2} \) are 3D vectors (e.g., \( [s_x, s_y, s_z] \)). The constraints remain the same but are solved in the 3D subspace.
  - Example constraint for matmul → ReLU:
    \[
    (s_{r1} b + s_{r2} i + s_{r3} j) \geq (s_{m1} b + s_{m2} i + s_{m3} j) + 1
    \]
    For all \( b, i, j \), this implies:
    \[
    s_{r1} \geq s_{m1}, \quad s_{r2} \geq s_{m2}, \quad s_{r3} \geq s_{m3}
    \]
    with at least one strict inequality to ensure progress.

- **Implementation**:
  - Use ISL to compute the scheduling cone for \( \mathbf{s_2} \) given \( \mathbf{s_1} \):
    ```cpp
    isl_set* computeDependentSchedule(isl_set* schedCone1, isl_map* depMap) {
        // Apply dependency constraints: s2 * p2 >= s1 * p1 + 1
        isl_set* constraints = isl_map_to_constraints(depMap, schedCone1);
        return isl_set_intersect(schedCone1, constraints);
    }
    ```
  - `schedCone1` is the scheduling cone for \( O_1 \) (from its SURE).
  - `depMap` is \( M_{12} \).
  - The result is the scheduling cone for \( O_2 \), restricted by \( O_1 \)’s schedule.

#### Step 3: Schedule Modulation with Output Availability
To account for the matmul’s schedule determining output availability:
- **Output Availability**: The matmul’s schedule \( \mathbf{s_m} \cdot [b, i, j] \) assigns a time to each \( C[b, i, j] \). For example:
  - If \( \mathbf{s_m} = [1, 0, 0] \), outputs are ordered by \( b \): all \( C[0, :, :] \) are available before \( C[1, :, :] \).
  - This ordering constrains when ReLU can start computing \( D[b, i, j] \).
- **Modulation Mechanism**: Adjust \( \mathbf{s_r} \) to “follow” \( \mathbf{s_m} \) by incorporating the dependency constraints into a joint scheduling problem.
  - Formulate a global scheduling cone for the entire DFG:
    - Collect all intra-operator constraints (from each operator’s SURE).
    - Collect all inter-operator constraints (from dependency maps).
    - Solve for all \( \mathbf{s_i} \) (one per operator) simultaneously:
      \[
      \text{minimize } \sum_i w_i \cdot \mathbf{s_i} \text{ subject to }
      \begin{cases}
        \text{intra-operator constraints for } O_i \\
        \mathbf{s_2} \cdot \mathbf{p_2} \geq \mathbf{s_1} \cdot \mathbf{p_1} + 1 \text{ for } (\mathbf{p_1}, \mathbf{p_2}) \in M_{12}
      \end{cases}
      \]
      where \( w_i \) are weights to prioritize certain operators (e.g., to optimize latency or throughput).
  - **Dynamic Adjustment**: If \( \mathbf{s_m} \) is fixed (e.g., precomputed), compute \( \mathbf{s_r} \) as a function of \( \mathbf{s_m} \):
    - Use the dependency map to project \( \mathbf{s_m} \)’s constraints onto \( \mathbf{s_r} \).
    - Example: For \( \mathbf{s_r} \cdot [b, i, j] \geq \mathbf{s_m} \cdot [b, i, j] + 1 \), solve for \( \mathbf{s_r} \) in the 3D cone.

- **Implementation**:
  - Use ISL’s linear programming solver to find a valid \( \mathbf{s_r} \):
    ```cpp
    isl_set* globalSchedule(DFG& dfg) {
        isl_set* globalCone = isl_set_universe(isl_space_alloc(ctx, 0, 3 * dfg.nodes.size()));
        for (const auto& node : dfg.nodes) {
            isl_set* intraCone = computeIntraSchedule(node.sure); // From SURE
            globalCone = isl_set_intersect(globalCone, intraCone);
        }
        for (const auto& edge : dfg.edges) {
            isl_set* interCone = computeDependentSchedule(
                dfg.nodes[edge.srcNodeId].schedule,
                edge.dependencyMap
            );
            globalCone = isl_set_intersect(globalCone, interCone);
        }
        return globalCone;
    }
    ```
  - Extract individual \( \mathbf{s_i} \) from `globalCone` using lexicographic minimization.

#### Step 4: Handle Fine-Grained SURE Embedding
- **3D Embedding**: The SURE’s fine-grained computation graph is embedded into 3D space, meaning each index point \( \mathbf{p} \) is mapped to a 3D coordinate (e.g., \( [x, y, z] \)). The scheduling vector \( \mathbf{s} \) operates in this 3D space.
- **Dependency Propagation**: When embedding dependencies (e.g., \( C[b, i, j] \rightarrow D[b, i, j] \)), map them to the 3D space:
  - If \( [b, i, j] \) maps to \( [x_b, y_i, z_j] \) in 3D, the dependency becomes:
    \[
    \mathbf{s} \cdot [x_b, y_i, z_j]_D \geq \mathbf{s} \cdot [x_b, y_i, z_j]_C + 1
    \]
  - Since \( [b, i, j] \) is identical for \( C \) and \( D \), the 3D coordinates are the same, simplifying the constraint.
- **Generalization**: For operators with complex embeddings (e.g., convolutions with shifted dependencies), compute the 3D coordinates of \( \mathbf{p_1} \) and \( \mathbf{p_2} \) using the embedding function and include them in the dependency map.

- **Implementation**:
  - Define an embedding function in the DFG node:
    ```cpp
    struct DomainFlowNode {
        isl_set* indexSpace;
        isl_map* sureDependencies;
        isl_map* embedding; // e.g., "{[b,i,j] -> [b,i,j]}" for simple cases
        isl_set* schedule;
    };
    ```
  - Apply the embedding to dependency maps:
    ```cpp
    isl_map* embedDependency(isl_map* depMap, isl_map* embed1, isl_map* embed2) {
        return isl_map_apply_domain(
            isl_map_apply_range(depMap, embed2), isl_map_reverse(embed1)
        );
    }
    ```

#### Step 5: Optimization and Flexibility
- **Optimize Schedules**: Use objectives like minimizing latency (maximize parallelism) or maximizing locality:
  - Latency: Minimize the maximum schedule time across all index points.
  - Locality: Prefer schedules where dependent points are close in 3D space.
- **Dynamic Scheduling**: If the matmul’s schedule changes (e.g., due to hardware constraints), recompute dependent schedules incrementally:
  - Cache intra-operator cones and update only inter-operator constraints.
- **Generalization**:
  - **Operator Types**: The mechanism works for any operator with a SURE (e.g., matmul, ReLU, convolution) as long as the dependency map is defined.
  - **Non-Pointwise Operators**: For operators like convolution, the dependency map includes shifts (e.g., \( [i, j] \rightarrow [i-1, j] \)), which are handled similarly.
  - **Multi-Operator Chains**: Extend to chains (e.g., matmul → ReLU → conv) by applying constraints transitively.

---

### 3. Practical Implementation Suggestions
Here’s how to integrate this mechanism into your C++ DFG framework:

1. **Data Structures**:
   - Extend your `DomainFlowNode` to include schedules and embeddings:
     ```cpp
     struct DomainFlowNode {
         std::string type; // "matmul", "relu", etc.
         isl_set* indexSpace;
         isl_map* sureDependencies;
         isl_map* embedding; // 3D embedding map
         isl_set* schedule; // Scheduling cone
     };
     ```
   - Store dependency maps in edges:
     ```cpp
     struct DFGEdge {
         int srcNodeId, dstNodeId;
         isl_map* dependencyMap;
     };
     ```

2. **Scheduling Algorithm**:
   - Compute intra-operator schedules using SURE dependencies (as you’re doing).
   - Add inter-operator constraints using the dependency maps.
   - Solve the global scheduling problem with ISL:
     ```cpp
     void computeSchedules(DFG& dfg) {
         for (auto& node : dfg.nodes) {
             node.schedule = computeIntraSchedule(node.sureDependencies);
         }
         isl_set* globalCone = globalSchedule(dfg); // From Step 3
         for (size_t i = 0; i < dfg.nodes.size(); ++i) {
             dfg.nodes[i].schedule = extractSchedule(globalCone, i);
         }
     }
     ```

3. **Testing**:
   - Create test cases for matmul → ReLU:
     ```cpp
     TEST(Scheduling, MatmulToReLU) {
         DFG dfg;
         int matmulId = dfg.addNode({"matmul", parseISLSet("{[b,i,j] : 0<=b<4, 0<=i<16, 0<=j<8}"), ...});
         int reluId = dfg.addNode({"relu", parseISLSet("{[b,i,j] : 0<=b<4, 0<=i<16, 0<=j<8}"), ...});
         dfg.addEdge(matmulId, reluId, parseISLMap("{[b,i,j] -> [b,i,j]}"));
         
         computeSchedules(dfg);
         
         auto s_m = dfg.nodes[matmulId].schedule;
         auto s_r = dfg.nodes[reluId].schedule;
         // Verify s_r >= s_m + 1 in the scheduling cone
         ASSERT_TRUE(checkDependencyConstraint(s_m, s_r, dfg.edges[0].dependencyMap));
     }
     ```
   - Test edge cases: empty index spaces, cyclic DFGs, non-pointwise dependencies.

4. **Tools**:
   - **ISL**: For polyhedral operations (sets, maps, cones).
   - **Google Test**: For unit tests.
   - **Eigen**: If you need to manipulate tensors for verification (e.g., matmul outputs).
   - **Valgrind**: To check for memory leaks in ISL usage.

---

### 4. Alternative Approaches
If the polyhedral approach is too heavyweight or you need simpler alternatives:

1. **Coarse-Grained Scheduling**:
   - Treat each operator as a single task (ignore fine-grained index points).
   - Schedule ReLU after matmul completes entirely:
     \[
     t_{\text{ReLU}} \geq t_{\text{matmul}} + \text{max}(\mathbf{s_m} \cdot \mathbf{p})
     \]
   - Pros: Simpler, fewer constraints.
   - Cons: Loses fine-grained parallelism (e.g., ReLU can’t start until all of matmul is done).

2. **Wavefront Scheduling**:
   - Use the matmul’s schedule to define “wavefronts” of available outputs.
   - Example: If \( \mathbf{s_m} = [1, 0, 0] \), outputs \( C[b, :, :] \) become available at times \( t = b \).
   - Schedule ReLU to follow the wavefront:
     \[
     \mathbf{s_r} = \mathbf{s_m} + \delta \cdot [1, 0, 0]
     \]
     where \( \delta \geq 1 \) is a delay.
   - Pros: Intuitive, aligns with batch ordering.
   - Cons: Less flexible than polyhedral approach.

3. **Event-Based Scheduling**:
   - Model output availability as events in the 3D space.
   - Use a dependency graph where nodes are index points and edges are dependencies.
   - Assign schedules based on topological order in the 3D-embedded graph.
   - Pros: Works for non-linear schedules.
   - Cons: Scales poorly with large index spaces.

The polyhedral approach is recommended because it aligns with your SURE framework, leverages ISL (which you’re likely using), and supports fine-grained parallelism, which is critical for your parallel algorithm goals.

---

### 5. Connection to Your Project
Drawing from your previous questions:
- **Tensor Shapes**: The matmul’s output tensor `(batchSize, m, n)` and ReLU’s input/output tensors have the same shape, simplifying the dependency map to an identity map. For other operators (e.g., convolution), you’d need to define more complex maps based on tensor strides or kernel sizes.
- **SURE for Matmul**: The matmul’s SURE (from your April 21, 2025 question) involves a reduction, which we approximate as a single output dependency per index point. This fits into the dependency map framework.
- **MLIR and DFA**: If you’re integrating this with your MLIR dialect (from March 5, 2025), you can encode dependency maps as attributes on DFG edges and use `mlir-opt` to analyze or transform the schedules.
- **Testing**: Extend your existing test framework (from your first question) to verify inter-operator scheduling constraints, using ISL to check cone intersections and constraint satisfaction.

---

### 6. Example Walkthrough
For a DFG with matmul → ReLU:
- **Matmul**:
  - Index space: \( \{[b, i, j] : 0 \leq b < 4, 0 \leq i < 16, 0 \leq j < 8\} \).
  - SURE: Reduction over \( p \), but output at \( [b, i, j] \).
  - Schedule: Suppose \( \mathbf{s_m} = [1, 0, 0] \) (order by batch).
- **ReLU**:
  - Index space: Same as above.
  - SURE: \( D[b, i, j] = \max(0, C[b, i, j]) \).
  - Dependency map: \( \{[b, i, j] \rightarrow [b, i, j]\} \).
- **Constraint**:
  \[
  \mathbf{s_r} \cdot [b, i, j] \geq \mathbf{s_m} \cdot [b, i, j] + 1
  \]
  \[
  (s_{r1} b + s_{r2} i + s_{r3} j) \geq (1 \cdot b + 0 \cdot i + 0 \cdot j) + 1
  \]
  \[
  s_{r1} \geq 1, \quad s_{r2} \geq 0, \quad s_{r3} \geq 0
  \]
- **Feasible \( \mathbf{s_r} \)**: Choose \( \mathbf{s_r} = [1, 0, 0] \) (same as matmul, with delay) or \( \mathbf{s_r} = [1, 1, 0] \) (prioritize \( i \)-ordering after \( b \)).
- **3D Embedding**: If \( [b, i, j] \mapsto [b, i, j] \), the constraint remains the same in 3D coordinates.

---

### 7. Future Extensions
- **Non-Linear Schedules**: If linear schedules are too restrictive, explore multi-dimensional schedules (e.g., \( \mathbf{s} \cdot \mathbf{p} + c \)) or tiled schedules.
- **Hardware Constraints**: Incorporate constraints for specific hardware (e.g., GPU thread blocks) into the scheduling cone.
- **Dynamic Updates**: Support runtime schedule adjustments based on partial output availability (e.g., for streaming computations).
- **Visualization**: Integrate with Graphviz to visualize the DFG and schedule ordering in 3D.

---

### 8. Conclusion
The proposed **dependency-driven scheduling framework** generalizes schedule modulation by:
1. Modeling inter-operator dependencies as polyhedral maps.
2. Propagating constraints from the supplying operator’s schedule to the dependent operator.
3. Solving a global scheduling problem in the 3D-embedded space using ISL.
4. Supporting arbitrary operators and dependency patterns through SUREs and index spaces.

This approach is flexible, leverages your existing polyhedral tools, and ensures fine-grained parallelism. For implementation, focus on extending your DFG data structures, integrating ISL for constraint solving, and testing with matmul → ReLU and other operator pairs.
