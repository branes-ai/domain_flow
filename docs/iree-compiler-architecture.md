# IREE Compiler Architecture

## Major components

### 1. **Entry Point: `ireeCompilerRunMain`**
   - This function serves as the main entry point for the compiler tool. It initializes the compiler environment, processes command-line arguments, and invokes the compilation pipeline.

   - In file iree-compile-main.cc, we find the entry point
```cpp
int main(int argc, char **argv) { 
  return ireeCompilerRunMain(argc, argv); 
}
```
   - In file IREECompileToolEntryPoint.cpp, we see the body of this function
```cpp
int ireeCompilerRunMain(int argc, char **argv) {
  // Inline the actual runIreecMain here as the single place to use it.
  return mlir::iree_compiler::runIreecMain(argc, argv);
}
```

### 2. **Compiler Components**
   
The IREE compiler is modular and consists of several key components:

1. **Frontend**
    - **Input Conversion**: Converts input models (e.g., TensorFlow, PyTorch, ONNX) into MLIR-based intermediate representations (IR). This involves dialects like `stablehlo` or `tosa`.
   - **Preprocessing**: Includes optimizations and transformations to prepare the input for further compilation stages.

2. **Core Compiler**
   - **MLIR Dialects**: The compiler uses custom MLIR dialects to represent different stages of the compilation process:
     - **Flow Dialect**: Represents high-level operations and data flow.
     - **HAL Dialect**: Represents hardware abstraction layer operations.
     - **Stream Dialect**: Models asynchronous execution and data streaming.
     - **VM Dialect**: Represents the final virtual machine bytecode.
   - **Pass Pipelines**: The compiler applies a series of transformation passes to lower the IR through these dialects, optimizing and refining the representation at each stage.

3. **Backend**
   - **Code Generation**: Translates the IR into hardware-specific code. This includes generating SPIR-V for GPUs, native code for CPUs, or other target-specific binaries.
   - **Target Abstraction**: The backend is designed to be retargetable, allowing support for new hardware architectures like your domain flow architecture.

### 3. **Abstractions and Extensibility**
   - **Pass Manager**: Manages the sequence of transformations applied to the IR. You can add custom passes for concurrency analysis or other optimizations.
   - **Dialect Extensions**: You can define new MLIR dialects or extend existing ones to represent domain-specific operations.
   - **Backend Plugins**: The backend supports plugins for adding new hardware targets, making it easier to integrate your domain flow architecture.

### 4. **Concurrency Analysis**
   - The Stream Dialect and its associated passes are particularly relevant for modeling and optimizing concurrency. You can extend these components to perform detailed concurrency analysis tailored to your needs.

### 5. **Runtime Integration**
   - The compiler generates modules that are executed by the IREE runtime. The runtime provides APIs for managing execution, memory, and hardware resources.

This modular architecture allows you to focus on specific components, such as adding new passes for concurrency analysis or extending the backend for your hardware target.

## Compiler targets and hardware targets

The IREE compiler is designed with portability in mind, enabling it to target a wide range of hardware configurations, including CPU-only and CPU-GPU setups. Here's how it achieves this:

1. **Unified Intermediate Representation (IR)**:
   - IREE lowers machine learning models into a unified IR that abstracts hardware-specific details. This IR is then transformed and optimized for specific hardware backends.

2. **Retargetable Backend Architecture**:
   - The compiler supports multiple backends, such as LLVM for CPUs, SPIR-V for GPUs, and other hardware-specific targets. Each backend generates code tailored to the target hardware.

3. **Executable Formats**:
   - IREE produces a single `.vmfb` (VM FlatBuffer) file that contains the compiled program. This file is hardware-agnostic and includes metadata and executables for all supported targets. At runtime, the IREE runtime selects the appropriate executable based on the available hardware.

4. **Runtime Flexibility**:
   - The IREE runtime dynamically adapts to the hardware configuration. For example:
     - On CPU-only systems, it uses the LLVM backend to execute the program.
     - On GPU-enabled systems, it leverages the SPIR-V backend for GPU acceleration.
   - This flexibility eliminates the need for separate `.vmfb` files for each target.

5. **Hardware Abstraction Layer (HAL)**:
   - The HAL provides a unified interface for interacting with different hardware backends. It abstracts the details of memory management, data transfer, and execution, ensuring portability across diverse hardware.

This integrated approach allows IREE to support a wide range of hardware configurations without requiring separate compilation artifacts for each target. 

## PassManager and adding a pass

The **Pass Manager** in IREE is a central component of the MLIR framework that orchestrates the transformation and optimization of intermediate representations (IR) through a sequence of passes. Here's a breakdown of how it works and how you can add custom passes:

### **Understanding the Pass Manager**
1. **Role**: The Pass Manager is responsible for managing the execution of passes, which are modular units of transformation applied to the IR. Passes can perform tasks like optimization, lowering, or analysis.
2. **Structure**: Passes are organized into pipelines, where each pipeline represents a sequence of passes tailored to achieve specific goals (e.g., lowering from one dialect to another).
3. **Execution**: The Pass Manager ensures that passes are executed in the correct order and handles dependencies between passes.

### **Adding Custom Passes**
To add a custom pass to the IREE compiler, follow these steps:

1. **Define the Pass**:
   - Create a new class that inherits from `mlir::PassWrapper` or `mlir::FunctionPass`.
   - Implement the `runOnFunction` or `runOnOperation` method to define the transformation logic.

   Example:
   ```cpp
   struct MyCustomPass : public mlir::PassWrapper<MyCustomPass, mlir::FunctionPass> {
       void runOnFunction() override {
           auto function = getFunction();
           // Custom transformation logic here
       }
   };
   ```

2. **Register the Pass**:
   - Use the `mlir::registerPass` function to register your pass with the Pass Manager.
   - This makes your pass available for inclusion in pipelines.

   Example:
   ```cpp
   mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
       return std::make_unique<MyCustomPass>();
   });
   ```

3. **Integrate the Pass into Pipelines**:
   - Modify the pipeline configuration to include your custom pass.
   - You can add your pass to existing pipelines or create a new pipeline.

   Example:
   ```cpp
   pm.addPass(std::make_unique<MyCustomPass>());
   ```

4. **Test and Debug**:
   - Use MLIR's debugging tools to test your pass and ensure it behaves as expected.
   - You can use flags like `--mlir-print-ir-after-all` to inspect the IR at different stages.

### **Extending the Compiler**
Custom passes can be used to extend the compiler for specific tasks, such as concurrency analysis or targeting new hardware architectures. By leveraging the modular design of the Pass Manager, you can integrate your custom logic seamlessly into the compilation pipeline.


### Optimization opportunities for custom passes

Custom passes in MLIR or IREE can be tailored to address specific needs in the compilation pipeline. Here are some examples of useful custom passes:

### 1. **Concurrency Analysis Pass**
   - **Purpose**: Analyze the IR to identify opportunities for parallel execution.
   - **Implementation**: Traverse the IR to detect independent operations or loops that can be parallelized. Annotate the IR with metadata for downstream passes or runtime execution.

### 2. **Memory Optimization Pass**
   - **Purpose**: Optimize memory usage by reducing redundant allocations or reusing buffers.
   - **Implementation**: Analyze buffer lifetimes and insert transformations to share or recycle memory buffers where possible.

### 3. **Custom Hardware Target Lowering**
   - **Purpose**: Lower operations to a custom dialect or hardware-specific instructions.
   - **Implementation**: Define a new dialect for your hardware and create a pass to convert standard operations into your custom dialect.

### 4. **Profiling Instrumentation Pass**
   - **Purpose**: Insert profiling hooks into the IR to measure performance metrics like execution time or memory usage.
   - **Implementation**: Add calls to profiling functions at key points in the IR, such as function entry/exit or loop boundaries.

### 5. **Domain-Specific Optimization Pass**
   - **Purpose**: Apply optimizations specific to a particular domain, such as image processing or natural language processing.
   - **Implementation**: Identify patterns in the IR that can be replaced with more efficient implementations, such as fusing operations or using specialized libraries.

### 6. **Error Detection and Debugging Pass**
   - **Purpose**: Add checks to the IR to detect runtime errors or validate assumptions.
   - **Implementation**: Insert runtime assertions or logging statements to catch issues during execution.

### 7. **Code Size Reduction Pass**
   - **Purpose**: Minimize the size of the generated code for resource-constrained environments.
   - **Implementation**: Remove dead code, inline small functions, and simplify complex operations.

These examples demonstrate the flexibility of the Pass Manager in MLIR and IREE. 

## Visualizing the DL Graph through a viz-pass

Creating a visualization pass involves several steps, combining MLIR's capabilities for graph traversal, shape analysis, and integration with external libraries for convex hull computation and 3D visualization. Here's a high-level breakdown of how you can approach this:

### **1. Define the Pass**
- **Purpose**: The pass will traverse the MLIR graph, identify core operators (e.g., Conv2D, SoftMax, Matmul), gather operands, perform shape analysis, compute loop bounds, and generate convex hulls.
- **Implementation**: Create a new pass inheriting from `mlir::OperationPass` or `mlir::FunctionPass`.

### **2. Graph Traversal**
- Use MLIR's operation traversal APIs to visit all nodes in the graph.
- Query the node name and attributes to identify the operator type (e.g., Conv2D, SoftMax, Matmul).
- Example:
  ```cpp
  void runOnOperation() override {
      getOperation()->walk([&](mlir::Operation *op) {
          if (auto convOp = dyn_cast<Conv2DOp>(op)) {
              // Process Conv2D operator
          }
      });
  }
  ```

### **3. Shape Analysis**
- Extract operand shapes and compute loop bounds based on the operator's semantics.
- Use MLIR's shape inference utilities or define custom logic for shape analysis.

### **4. Convex Hull Computation**
- Integrate a library like Qhull or CGAL to compute the convex hull for the constraints defined by the loop bounds.
- Example:
  ```cpp
  // Pseudo-code for convex hull computation
  std::vector<Point> points = computeLoopBounds(op);
  ConvexHull hull = computeConvexHull(points);
  ```

### **5. 3D Scene Graph Generation**
- Use a 3D visualization library (e.g., OpenGL, Vulkan, or a higher-level library like Three.js) to create a scene graph representing the convex hull.
- Export the convex hull data in a format compatible with the visualization library.

### **6. GUI Integration**
- Develop a GUI using a framework like Qt or Dear ImGui to manage the scene graph.
- Enable user interaction for interrogating operators and modifying parallelism.

### **7. Pass Registration**
- Register the pass with MLIR's Pass Manager and integrate it into the compilation pipeline.

### **8. Testing and Debugging**
- Test the pass with various DL graphs to ensure correctness and robustness.
- Use MLIR's debugging tools to inspect the IR and verify transformations.

## Creating a KPU Dialect

Creating a new MLIR dialect for the Knowledge Processing Unit (KPU) involves defining operations, types, and attributes that represent the KPU's resources and execution model. Here's a step-by-step guide to help you design and implement this dialect:

---

### **1. Define the Dialect**
- Create a new dialect class inheriting from `mlir::Dialect`.
- Define the namespace for your dialect (e.g., `kpu`).
- Register operations, types, and attributes specific to the KPU.

Example:
```cpp
#include "mlir/IR/Dialect.h"

namespace mlir {
namespace kpu {

class KPUDialect : public Dialect {
public:
  explicit KPUDialect(MLIRContext *context)
      : Dialect("kpu", context, TypeID::get<KPUDialect>()) {
    // Register operations, types, and attributes here.
    addOperations<
        // List of operations
    >();
    addTypes<
        // List of types
    >();
  }
};

} // namespace kpu
} // namespace mlir
```

---

### **2. Define KPU Resources**
For the KPU's resources (local memory, scratchpad memories, mesh router, program memories), define custom types and attributes:

#### **a. Types**
- Define types for local memory, scratchpad memories, and program memories.
- Use `mlir::Type` to represent these resources.

Example:
```cpp
class LocalMemoryType : public Type::TypeBase<LocalMemoryType, Type, TypeStorage> {
  // Implementation for local memory type.
};

class ScratchpadMemoryType : public Type::TypeBase<ScratchpadMemoryType, Type, TypeStorage> {
  // Implementation for scratchpad memory type.
};
```

#### **b. Attributes**
- Define attributes for properties like memory size, topology, and routing configurations.
- Use `mlir::Attribute` to represent these properties.

Example:
```cpp
class MeshRouterAttr : public Attribute::AttrBase<MeshRouterAttr, Attribute, AttributeStorage> {
  // Implementation for mesh router attributes.
};
```

---

### **3. Define Operations**
Create operations to represent the KPU's functionality, such as:
- **Memory Allocation**: Allocate and manage local and scratchpad memories.
- **Data Movement**: Define operations for data transfer through the mesh router.
- **Program Execution**: Represent domain flow programs and their execution.

Example:
```cpp
class AllocateMemoryOp : public Op<AllocateMemoryOp, OpTrait::ZeroOperands, OpTrait::OneResult> {
public:
  static StringRef getOperationName() { return "kpu.allocate_memory"; }

  // Define operands, results, and attributes.
};
```

---

### **4. Represent the Domain Flow Compute Fabric**
- Define operations and attributes to model the programmable topology and size of the domain flow compute fabric.
- Use MLIR's region and block constructs to represent the flow of computation.

Example:
```cpp
class DomainFlowOp : public Op<DomainFlowOp, OpTrait::VariadicOperands, OpTrait::VariadicResults> {
public:
  static StringRef getOperationName() { return "kpu.domain_flow"; }

  // Define regions for representing the flow of computation.
};
```

---

### **5. Integrate with MLIR**
- Register the dialect with the MLIR context.
- Add the dialect to the Pass Manager to enable transformations and optimizations.

Example:
```cpp
void registerKPUDialect(DialectRegistry &registry) {
  registry.insert<mlir::kpu::KPUDialect>();
}
```

---

### **6. Visualization and Debugging**
- Implement custom printing and parsing methods for your dialect to make it easier to debug and visualize.
- Use MLIR's `AsmPrinter` and `AsmParser` utilities.

---

### **7. Testing and Validation**
- Write unit tests to validate the correctness of your dialect.
- Use MLIR's testing framework to ensure compatibility with the rest of the compiler.

---

This approach provides a foundation for representing the KPU and its resources in MLIR. 

## Detailed design of a KPU Dialect

To define the operations and types for the Knowledge Processing Unit (KPU) in MLIR, we need to model its resources and execution semantics effectively. Here's a detailed breakdown:

---

### **1. Define KPU Types**
KPU types represent the resources and data structures used by the KPU, such as local memory, scratchpad memories, and program memories.

#### **a. Custom Types**
- Use `mlir::Type` to define custom types for KPU resources.
- Example: Define types for local memory and scratchpad memory.

```cpp
class LocalMemoryType : public Type::TypeBase<LocalMemoryType, Type, TypeStorage> {
  // Implementation for local memory type.
};

class ScratchpadMemoryType : public Type::TypeBase<ScratchpadMemoryType, Type, TypeStorage> {
  // Implementation for scratchpad memory type.
};
```

#### **b. Attributes for Types**
- Add attributes to specify properties like memory size, topology, or routing configurations.
- Example: Define an attribute for memory size.

```cpp
class MemorySizeAttr : public Attribute::AttrBase<MemorySizeAttr, Attribute, AttributeStorage> {
  // Implementation for memory size attribute.
};
```

---

### **2. Define KPU Operations**
KPU operations represent the actions performed by the KPU, such as memory allocation, data movement, and program execution.

#### **a. Memory Allocation Operation**
- Represents the allocation of local or scratchpad memory.

```cpp
class AllocateMemoryOp : public Op<AllocateMemoryOp, OpTrait::ZeroOperands, OpTrait::OneResult> {
public:
  static StringRef getOperationName() { return "kpu.allocate_memory"; }

  // Define operands, results, and attributes.
};
```

#### **b. Data Movement Operation**
- Represents data transfer through the mesh router.

```cpp
class DataTransferOp : public Op<DataTransferOp, OpTrait::VariadicOperands, OpTrait::VariadicResults> {
public:
  static StringRef getOperationName() { return "kpu.data_transfer"; }

  // Define operands, results, and attributes.
};
```

#### **c. Program Execution Operation**
- Represents the execution of a domain flow program.

```cpp
class ExecuteProgramOp : public Op<ExecuteProgramOp, OpTrait::VariadicOperands, OpTrait::VariadicResults> {
public:
  static StringRef getOperationName() { return "kpu.execute_program"; }

  // Define regions for representing the flow of computation.
};
```

---

### **3. Representing the Domain Flow Compute Fabric**
The domain flow compute fabric is a generalization of the systolic array. To model this:

#### **a. Define a Custom Dialect**
- Create a dialect class for the KPU, inheriting from `mlir::Dialect`.
- Register the operations and types within this dialect.

```cpp
class KPUDialect : public Dialect {
public:
  explicit KPUDialect(MLIRContext *context)
      : Dialect("kpu", context, TypeID::get<KPUDialect>()) {
    addOperations<AllocateMemoryOp, DataTransferOp, ExecuteProgramOp>();
    addTypes<LocalMemoryType, ScratchpadMemoryType>();
  }
};
```

#### **b. Define Topology and Size**
- Use attributes to specify the topology and size of the compute fabric.
- Example: Define an attribute for the mesh router configuration.

```cpp
class MeshRouterAttr : public Attribute::AttrBase<MeshRouterAttr, Attribute, AttributeStorage> {
  // Implementation for mesh router attributes.
};
```

---

### **4. Integration and Testing**
- **Integration**: Register the KPU dialect with the MLIR context and integrate it into the compilation pipeline.
- **Testing**: Write unit tests to validate the correctness of the operations and types.

---

This approach provides a foundation for modeling the KPU's resources and execution semantics in MLIR.

## Profiling Instrumentation

A great reference point for profiling instrumentation in IREE is its integration with the **Tracy profiler**, which is used for performance analysis. Tracy is a hybrid instrumentation and sampling profiler, and IREE's runtime is instrumented to support it. Here's an overview of how profiling is implemented in IREE:

1. **Instrumentation in IREE**:
   - IREE uses macros defined in `iree/base/tracing.h` to instrument its runtime code. These macros allow developers to define zones for profiling, marking the start and end of specific code regions.
   - Example:
     ```cpp
     void iree_sample_function() {
         IREE_TRACE_ZONE_BEGIN(z0);
         // Code to be profiled
         IREE_TRACE_ZONE_END(z0);
     }
     ```

2. **Tracy Integration**:
   - Tracy operates in a client-server model. The "client" is the program being profiled (e.g., `iree-run-module`), and the "server" is the Tracy profiler UI or command-line capture tool.
   - The profiling data is captured and visualized in the Tracy UI, allowing developers to analyze performance bottlenecks.

3. **Building with Profiling Support**:
   - To enable profiling, IREE must be built with tracing support. This is done by setting the `-DIREE_ENABLE_RUNTIME_TRACING=ON` flag during the build process.
   - Example:
     ```bash
     cmake -G Ninja -B ../iree-build/ -S . \
         -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DIREE_ENABLE_RUNTIME_TRACING=ON
     cmake --build ../iree-build/ --target iree-run-module
     ```

4. **Capturing a Trace**:
   - Once built, you can use tools like `iree-run-module` to execute a program and capture profiling data. The Tracy profiler UI or CLI tool is used to visualize the captured data.

For more details, you can explore the [official IREE documentation on profiling with Tracy](https://iree.dev/developers/performance/profiling-with-tracy/). This example demonstrates how profiling instrumentation can be integrated into a runtime, providing a solid foundation for creating custom profiling passes. 

## Designing an instrumentation pass for arithmetic complexity

Designing a custom pass modeled after LLVM's `instcount` pass, which analyzes and counts instructions in IR, is a great starting point to compute the arithmetic complexity of a graph in IREE/MLIR. Here's how you can approach building such a pass:

---

### **Purpose**
This pass will:
1. Traverse the IR to visit all operations in the computational graph.
2. Identify and count arithmetic operations such as `add`, `mul`, `div`, `sqrt`, etc.
3. Aggregate arithmetic complexity (e.g., number of floating-point operations, FLOPs).
4. Optionally, annotate the IR with metadata or print a summary of the complexity for debugging or reporting.

---

### **Implementation Steps**

#### **1. Create a Pass Skeleton**
Create a new pass file (e.g., `ArithmeticComplexityPass.cpp`) in the appropriate folder of the IREE or MLIR repository.

- Inherit from `mlir::PassWrapper` and implement the `runOnOperation` method.
- Example code skeleton:
```cpp
#include "mlir/Pass/Pass.h"
#include "mlir/IR/Operation.h"

namespace mlir {
namespace iree {

// Define the pass
class ArithmeticComplexityPass
    : public PassWrapper<ArithmeticComplexityPass, OperationPass<ModuleOp>> {
public:
  StringRef getArgument() const override { return "iree-arithmetic-complexity"; }
  StringRef getDescription() const override {
    return "Computes arithmetic complexity of the MLIR graph.";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    // Initialize counters
    int64_t addCount = 0, mulCount = 0, divCount = 0, totalOps = 0;

    // Traverse operations
    module.walk([&](Operation *op) {
      if (isa<AddFOp>(op)) {
        addCount++;
      } else if (isa<MulFOp>(op)) {
        mulCount++;
      } else if (isa<DivFOp>(op)) {
        divCount++;
      }
      // Count all operations
      totalOps++;
    });

    // Print the results
    llvm::outs() << "Arithmetic Complexity Report:\n";
    llvm::outs() << "Add operations: " << addCount << "\n";
    llvm::outs() << "Mul operations: " << mulCount << "\n";
    llvm::outs() << "Div operations: " << divCount << "\n";
    llvm::outs() << "Total operations: " << totalOps << "\n";
  }
};

// Register the pass
std::unique_ptr<Pass> createArithmeticComplexityPass() {
  return std::make_unique<ArithmeticComplexityPass>();
}

static PassRegistration<ArithmeticComplexityPass> pass;

} // namespace iree
} // namespace mlir
```

---

#### **2. Identify Arithmetic Operations**
- Extend the traversal logic to include operations relevant to your analysis.
- For example, in MLIR, floating-point and integer arithmetic operations like `AddFOp`, `AddIOp`, `MulFOp`, and `MulIOp` are part of the Standard Dialect.

#### **3. Aggregate Complexity**
- If additional complexity metrics are required (e.g., FLOPs for tensors or nested loops), add logic to extract operand shapes and compute the operation's contribution to overall complexity.
- Example: For matrix multiplication, compute complexity as `M x N x K` based on tensor dimensions.

---

#### **4. Annotate the IR (Optional)**
- Add metadata or attributes to the operations to store the computed complexity.
- Example:
```cpp
op->setAttr("complexity", IntegerAttr::get(IntegerType::get(op->getContext(), 64), opComplexity));
```

#### **5. Generate a Summary Report**
- Use `llvm::outs()` to output the report, or write it to a file for further analysis.

---

### **Testing and Debugging**
1. Write test cases in MLIR's `.mlir` format to verify the pass.
2. Example test case:
```mlir
func @test() {
  %0 = addf %arg0, %arg1 : f32
  %1 = mulf %arg2, %arg3 : f32
  return
}
```
3. Run the pass using `iree-opt`:
```bash
iree-opt --iree-arithmetic-complexity test.mlir
```

---

This approach captures the essence of LLVM's `instcount` pass, while tailoring it to arithmetic complexity analysis for MLIR.

