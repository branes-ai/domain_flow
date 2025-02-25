# ONNX runtime architecture

Let's break down the ONNX Runtime architecture, focusing on its graph representation, execution, and the role of the optimizing compiler.

**1. Graph Representation:**

* **ONNX Graph:** At its core, ONNX Runtime uses the ONNX graph representation. This is a directed acyclic graph (DAG) where nodes represent operators (like Add, MatMul, Conv), and edges represent data flow (tensors).  The ONNX graph is defined in the ONNX protobuf schema.  This is the initial, high-level representation.

* **In-Memory Representation:** When an ONNX model is loaded into ONNX Runtime, it's parsed and converted into an internal, optimized graph representation. This internal representation is more suitable for execution and manipulation by the runtime. While it still represents the same computational logic, it may be transformed for efficiency (e.g., node fusion, layout optimizations).  This internal representation is where much of the optimization happens.  It's not usually the ONNX protobuf directly.

* **Partitions and Execution Providers:**  The graph is then partitioned based on *execution providers*.  Different parts of the graph can run on different hardware (CPU, GPU, specialized accelerators).  The partitioner assigns nodes to the most appropriate provider.  Each partition is a sub-graph.

**2. Execution:**

* **Session:** The `InferenceSession` class is the primary interface for running inference. It loads the ONNX model, performs graph optimizations, and manages execution.

* **Providers:** Each execution provider (e.g., CPU, CUDA, TensorRT) has its own implementation of the operators it supports.  These implementations are highly optimized for the specific hardware.

* **Kernel Dispatch:** During execution, the runtime traverses the graph. For each node, it dispatches the corresponding kernel (the provider-specific implementation of the operator) to the appropriate execution provider.

* **Data Flow:** Tensors flow between operators according to the graph's edges. The runtime manages tensor allocation and memory management.

**3. Optimizing Compiler:**

* **Graph Transformations:**  The compiler performs a series of graph transformations to improve performance. These transformations work on the internal graph representation, *not* the original ONNX protobuf directly.  Common optimizations include:
    * **Node Fusion:** Combining multiple operations into a single kernel (e.g., fusing a series of element-wise operations).
    * **Constant Folding:** Evaluating constant expressions at compile time.
    * **Layout Optimization:**  Adjusting the memory layout of tensors for better cache utilization.
    * **Shape Inference:** Inferring tensor shapes where possible to enable further optimizations.
    * **Common Subexpression Elimination:** Identifying and eliminating redundant computations.
    * **Allocator Optimization:** Optimizing tensor allocation and reuse to minimize memory footprint and allocations.

* **Code Generation (Sometimes):** Some execution providers might use code generation (e.g., generating optimized CUDA kernels).  This is less common for CPU execution but very frequent for GPUs.

* **Impact on Abstraction:** The compiler *enhances* the computational abstraction.  It doesn't fundamentally alter the basic operator-based graph representation, but it creates a more efficient version of it. The compiler works *below* the level of the ONNX operators, manipulating the graph to produce a functionally equivalent but faster execution plan.  It might fuse operators, but the *idea* of the operators is still there. The compiler is concerned with transforming the *how* of the computation, not the *what*.

* **Customization:** ONNX Runtime provides options to control the optimization level.

**Example:**

Imagine a graph with `Add`, `Mul`, and `Sub` operations. The compiler might fuse these into a single custom kernel if they are element-wise operations.  From the user's perspective, the graph still conceptually performs addition, multiplication, and subtraction. But under the hood, the runtime executes a single, fused kernel, which is much faster.

**Key Points:**

* The ONNX graph is the starting point, but ONNX Runtime works with a more optimized internal representation.
* Execution providers handle the actual computation.
* The optimizing compiler transforms the graph to improve performance, working below the level of individual ONNX operations to optimize the execution plan.  It makes the execution more efficient without changing the computational logic described by the graph.


