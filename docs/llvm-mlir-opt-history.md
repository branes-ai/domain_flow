# LLVM optimizer history

This document describes the history of `llvm-opt` and `mlir-opt`, their roles within the LLVM and MLIR ecosystems, and how they can be leveraged to study and develop analysis passes for deep learning (DL) compilers, particularly in the context of extending MLIR with a new dialect for domain flow architecture principles.

### History of `llvm-opt` and `mlir-opt`

#### `llvm-opt`: The LLVM Optimizer Tool
`llvm-opt` (often simply referred to as `opt` in the LLVM community) originated as part of the LLVM project, which began in 2000 as a research effort by Chris Lattner at the University of Illinois. LLVM aimed to provide a modular, reusable compiler infrastructure with a focus on optimization and code generation. `opt` emerged as a standalone tool designed to apply optimization and analysis passes to LLVM Intermediate Representation (IR) files, allowing developers to test and debug transformations without needing to invoke the full compiler pipeline.

- **Purpose**: `llvm-opt` takes LLVM IR (typically in human-readable `.ll` format or bitcode `.bc` format) as input, runs a sequence of passes specified by the user, and outputs the transformed IR. It’s primarily a testing and development tool for LLVM pass developers.
- **Evolution**: Over time, `opt` grew to support a wide range of passes, including classic optimizations (e.g., constant propagation, dead code elimination) and analyses (e.g., alias analysis, dominator tree construction). It became a cornerstone for experimenting with LLVM IR transformations, especially as LLVM expanded to support diverse targets like CPUs, GPUs, and eventually domain-specific accelerators.
- **Key Features**:
  - Pass pipeline configuration via command-line flags (e.g., `-O3`, `-loop-unroll`).
  - Debugging support with options like `--print-after-all` to inspect IR after each pass.
  - Integration with LLVM’s pass manager, which evolved from a legacy design to a more modular "New Pass Manager" in recent years (circa LLVM 9, 2019).

While `llvm-opt` excels at low-level optimizations and is widely used for traditional compiler workflows, its fixed IR structure and lack of extensibility for high-level abstractions (e.g., DL graphs) led to the development of MLIR, where `mlir-opt` would play a similar but more flexible role.

#### `mlir-opt`: The MLIR Optimizer Tool
`mlir-opt` is a more recent tool, introduced as part of the MLIR (Multi-Level Intermediate Representation) project, which was initiated around 2017-2018 by Chris Lattner and others at Google, building on lessons from LLVM. MLIR was designed to address the fragmentation in machine learning (ML) and high-performance computing (HPC) compiler ecosystems, where frameworks like TensorFlow had disparate IRs and optimization pipelines. MLIR’s first public announcement came in 2019 via the TensorFlow blog, and it was later integrated into the LLVM project as a subproject.

- **Purpose**: Like `llvm-opt`, `mlir-opt` is a command-line tool for running passes on MLIR IR, but it’s tailored to MLIR’s extensible, multi-level design. It loads MLIR IR (textual `.mlir` or bytecode), applies transformations or analyses, and outputs the result, making it ideal for testing and developing passes across MLIR’s diverse dialects.
- **Evolution**:
  - MLIR’s design emphasizes dialects—modular, user-definable IR subsets—allowing it to represent high-level DL graphs (e.g., TensorFlow’s IR) down to low-level machine code (e.g., LLVM IR). `mlir-opt` evolved to support this flexibility, becoming the primary tool for pass developers.
  - Early MLIR adopters, particularly in the TensorFlow ecosystem, used `mlir-opt` to prototype transformations like quantization and kernel fusion. Its role expanded as MLIR gained traction in projects like IREE, Torch-MLIR, and Polygeist.
  - By 2025, `mlir-opt` has become a central hub for the MLIR community, supporting a rich ecosystem of dialects (e.g., `linalg`, `affine`, `tosa`) and passes for DL, HPC, and hardware-specific optimizations.
- **Key Features**:
  - Dialect-aware pass execution, enabling transformations across abstraction levels.
  - Advanced debugging with flags like `--mlir-print-ir-after-all`, `--mlir-timing`, and `--mlir-pass-statistics`.
  - Support for custom dialects, making it a playground for experimenting with new IR designs like your proposed domain flow architecture dialect.

### Why the Community Uses `mlir-opt` for DL Graphs

The MLIR community relies heavily on `mlir-opt` for interacting with DL graphs because of its flexibility and alignment with MLIR’s goals:

1. **Extensibility**: Unlike `llvm-opt`, which operates on a fixed IR, `mlir-opt` supports custom dialects. This allows developers to define DL-specific operations (e.g., convolutions, matrix multiplications) and transformations tailored to frameworks like TensorFlow or PyTorch.
2. **Multi-Level Transformations**: MLIR’s ability to represent graphs at multiple abstraction levels (e.g., high-level TensorFlow graphs, mid-level `linalg` ops, low-level LLVM IR) means `mlir-opt` can apply passes that bridge these layers, crucial for DL compiler development.
3. **Pass Development Workflow**: `mlir-opt` provides a lightweight, iterative environment to test new passes without recompiling an entire compiler, making it ideal for rapid prototyping of DL graph analyses (e.g., dataflow optimizations, loop tiling).
4. **Community Adoption**: Projects like Torch-MLIR and IREE use `mlir-opt` to lower DL models into optimized IRs, leveraging its integration with dialects like `linalg` and `affine` for polyhedral-style optimizations—your use case of replacing/augmenting these with domain flow principles fits this paradigm.

### Leveraging `llvm-opt` and `mlir-opt` for DL Compiler Analysis Passes

To study and develop analysis passes for your new dialect based on domain flow architecture principles (e.g., systems of affine recurrence equations), you can use these tools as follows:

#### Step 1: Define Your Dialect
Assume you’re creating a `domainflow` dialect to represent DL graphs with operations tied to recurrence equations (e.g., `domainflow.recur` for iterative computations). You’d define this using MLIR’s Operation Definition Specification (ODS) and register it with the MLIR ecosystem.

Example (simplified ODS):
```tablegen
def DomainFlowDialect : Dialect {
  let name = "domainflow";
  let summary = "Dialect for domain flow architecture analysis";
}

def DomainFlow_RecurOp : Op<DomainFlowDialect, "recur"> {
  let arguments = (ins AnyType:$input, IndexType:$steps);
  let results = (outs AnyType:$output);
  let summary = "Recurrence operation for domain flow";
}
```

#### Step 2: Use `mlir-opt` to Test Your Dialect
Write a simple MLIR file (`test.mlir`) to exercise your dialect:
```mlir
func.func @simple_recur(%input: tensor<4xf32>) -> tensor<4xf32> {
  %steps = arith.constant 3 : index
  %output = domainflow.recur %input, %steps : tensor<4xf32>, index -> tensor<4xf32>
  return %output : tensor<4xf32>
}
```

Run it through `mlir-opt` to verify parsing:
```bash
mlir-opt test.mlir -o test_roundtrip.mlir
```
This ensures your dialect’s syntax is correct and can be round-tripped.

#### Step 3: Develop an Analysis Pass
Create an analysis pass to study properties of your DL graph, such as identifying recurrence patterns or data dependencies. For example, a pass to count recurrence steps:

```cpp
#include "mlir/Pass/Pass.h"
#include "mlir/IR/BuiltinOps.h"

namespace mlir {
namespace domainflow {

struct RecurAnalysisPass : public PassWrapper<RecurAnalysisPass, OperationPass<FuncOp>> {
  void runOnOperation() override {
    FuncOp func = getOperation();
    func.walk([&](Operation *op) {
      if (auto recurOp = dyn_cast<RecurOp>(op)) {
        Attribute stepsAttr = recurOp.getSteps();
        if (auto steps = stepsAttr.dyn_cast<IntegerAttr>()) {
          llvm::outs() << "Found recur op with steps: " << steps.getInt() << "\n";
        }
      }
    });
  }
};

std::unique_ptr<Pass> createRecurAnalysisPass() {
  return std::make_unique<RecurAnalysisPass>();
}

} // namespace domainflow
} // namespace mlir
```

Register the pass in your MLIR setup and run it with `mlir-opt`:
```bash
mlir-opt test.mlir -pass-pipeline="builtin.module(func.func(domainflow-recur-analysis))"
```
Output might look like:
```
Found recur op with steps: 3
```

#### Step 4: Augment with Transformation Passes
To replace polyhedral approaches (e.g., `affine` dialect), develop a transformation pass to convert `linalg` ops into your `domainflow` ops. For instance, transform a matrix multiplication into a recurrence form:

```cpp
struct LinalgToDomainFlow : public OpRewritePattern<linalg::MatmulOp> {
  using OpRewritePattern<linalg::MatmulOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::MatmulOp matmul, PatternRewriter &rewriter) const override {
    Value inputA = matmul.getInputs()[0];
    Value inputB = matmul.getInputs()[1];
    Value steps = rewriter.create<arith::ConstantIndexOp>(matmul.getLoc(), 1);
    Value output = rewriter.create<domainflow::RecurOp>(
        matmul.getLoc(), inputA.getType(), inputA, steps);
    rewriter.replaceOp(matmul, output);
    return success();
  }
};
```

Run it with:
```bash
mlir-opt input.mlir -convert-linalg-to-domainflow -o output.mlir
```

#### Step 5: Compare with `llvm-opt`
If you lower your `domainflow` dialect to LLVM IR (via a lowering pass), use `llvm-opt` to analyze the resulting low-level IR:
```bash
mlir-opt output.mlir -lower-to-llvm | llvm-opt -S -o output.ll
llvm-opt output.ll -analyze -scalar-evolution
```
This could reveal how your recurrence equations map to loops, helping you refine your high-level analysis.

### Practical Examples from the Community
- **TensorFlow Graph Optimization**: The TensorFlow MLIR team uses `mlir-opt` to test passes like constant folding on the `tf` dialect, e.g., `mlir-opt -tf-constant-folding`.
- **Linalg Optimizations**: The `linalg` dialect leverages `mlir-opt` for fusion and tiling passes (`-linalg-fusion`), which you could adapt for domain flow principles.
- **IREE**: IREE uses `mlir-opt` to lower DL models to hardware-specific IRs, providing a model for integrating your dialect with backends.

### Conclusion
`llvm-opt` laid the groundwork for pass-based optimization in LLVM, but `mlir-opt` extends this to the multi-level, extensible world of MLIR, making it the go-to tool for DL graph analysis and transformation. By using `mlir-opt` to test your `domainflow` dialect, develop analysis passes, and integrate with existing DL dialects like `linalg`, you can explore domain flow architecture principles effectively. Start small with parsing and analysis, then scale to transformations, using `llvm-opt` as a secondary tool for low-level insights. This iterative approach mirrors how the MLIR community has successfully built out its ecosystem!


* **`llvm-opt`:**
    * `llvm-opt` is a command-line tool within the LLVM project. It's designed to run LLVM's optimization passes on LLVM Intermediate Representation (LLVM IR).
    * It's a core tool for working with LLVM IR, allowing developers to inspect, transform, and optimize code at the LLVM IR level.
    * Historically, it's been essential for compiler development, performance analysis, and optimization tuning for general-purpose code.
* **`mlir-opt`:**
    * `mlir-opt` is the MLIR equivalent of `llvm-opt`. It's a command-line tool that applies MLIR passes to MLIR code.
    * MLIR is a more flexible and extensible infrastructure than LLVM IR, designed to support a wider range of compilation tasks, including hardware acceleration, domain-specific languages (DSLs), and deep learning compilation.
    * `mlir-opt` allows developers to work with MLIR at various levels of abstraction, from high-level dialects representing DSLs to lower-level dialects closer to hardware.
    * It's crucial for developing, testing, and debugging MLIR passes, transformations, and optimizations.
    * Essentially, as MLIR has been designed to replace the need to go directly to LLVM IR in many use cases, `mlir-opt` has taken the place of `llvm-opt` for those workflows.

**Why `mlir-opt` is Instrumental in MLIR Development**

* **Pass Pipeline Execution:**
    * `mlir-opt` allows you to define and execute pipelines of MLIR passes. This is essential for building complex transformations and optimizations.
    * You can specify the order in which passes are run, control their parameters, and observe the effects of each pass on the MLIR code.
* **Dialect Transformation and Analysis:**
    * MLIR's power lies in its dialects, which represent different levels of abstraction and domain-specific concepts.
    * `mlir-opt` enables you to transform and analyze MLIR code within and across dialects.
    * For example, you can use `mlir-opt` to lower a high-level dialect representing a DL graph to a lower-level dialect representing hardware instructions.
* **Debugging and Inspection:**
    * `mlir-opt` provides options for printing MLIR code at various stages of the compilation process.
    * This allows you to inspect the intermediate representations and debug your passes.
    * You can also use options to verify the correctness of your transformations.
* **Testing:**
    * The tool is heavily used for testing new passes. By creating input files containing MLIR, and then using mlir-opt to apply a pass, and then comparing the output to expected results, developers can insure the correctness of their code.

**Leveraging `mlir-opt` for DL Graph Analysis and Pass Development**

## Develop analysis passes for DL compilers

Here's how you can use `mlir-opt` to study and develop analysis passes for DL compilers, particularly in the context of your domain flow architecture and affine recurrence equations:

1.  **Creating and Inspecting MLIR Input:**
    * Start by creating MLIR input files that represent your DL graphs. You can use the textual format of MLIR to describe your graphs using your new dialect.
    * Use `mlir-opt -print-ir-after-all <input.mlir>` to print the MLIR code after each pass in the pipeline. This helps you understand how the code is transformed.
    * For example, you could create a file that represents a convolutional layer, and then use mlir-opt to examine how your new affine recurrence based dialect represents that layer.

2.  **Developing Analysis Passes:**
    * Write your analysis passes in C++. These passes can traverse the MLIR IR, analyze the operations in your dialect, and extract information about affine recurrence equations.
    * Use the MLIR APIs to navigate the IR, access operation attributes, and perform dataflow analysis.
    * For Example, you could write a pass that parses your new dialect, and then determines the data dependancies between operations based on the affine recurrence equations.
    * Register your passes with the MLIR pass manager.

3.  **Building and Running Pass Pipelines:**
    * Use `mlir-opt` to create pass pipelines that include your analysis passes.
    * For example:
        * `mlir-opt <input.mlir> -my-analysis-pass -print-ir-after-all`
        * Where `-my-analysis-pass` is the name of your registered analysis pass.
    * You can chain multiple passes together to perform complex analyses and transformations.
    * You can also create `.mlir` files that contain the pass pipelines, and then use `mlir-opt -p <pipeline.mlir> <input.mlir>` to execute them.

4.  **Verifying and Debugging:**
    * Use `mlir-opt -verify-diagnostics` to check for errors and warnings in your MLIR code and passes.
    * Use the MLIR debugger or print statements to inspect the state of your passes and the IR.
    * Create test cases that exercise different aspects of your analysis passes, and use `mlir-opt` to run them and compare the results.

5.  **Integrating with DL Compilers:**
    * Once your analysis passes are working, you can integrate them into your DL compiler pipeline.
    * This involves linking your passes with the compiler and configuring the pass manager to run them.

**Example Scenario**

Let's say you have a new dialect called `affrec` that represents affine recurrence equations. You want to analyze the data dependencies in a DL graph represented in this dialect.

Step 1.  **Create `input.mlir`:**

```mlir
module {
  func @main() {
    %0 = affrec.load %arg0[%i, %j] : tensor<10x10xf32>
    %1 = affrec.add %0, %arg1[%i + 1, %j] : tensor<10x10xf32>
    affrec.store %1, %arg2[%i, %j] : tensor<10x10xf32>
    return
  }
}
```

Step 2.  **Create `my-analysis-pass.cpp`:**

```cpp
// ... (MLIR pass code to analyze data dependencies) ...
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Operation.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {

struct AffRecAnalysisPass : public PassWrapper<AffRecAnalysisPass, OperationPass<FuncOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(AffRecAnalysisPass)

  void runOnOperation() override {
    FuncOp func = getOperation();

    func.walk([&](Operation *op) {
      if (op->getName().getStringRef() == "affrec.load") {
        analyzeLoad(op);
      } else if (op->getName().getStringRef() == "affrec.add") {
        analyzeAdd(op);
      } else if (op->getName().getStringRef() == "affrec.store") {
        analyzeStore(op);
      }
    });
  }

  void analyzeLoad(Operation *op) {
    llvm::outs() << "Analyzing affrec.load operation:\n";
    // Access operands and attributes of the load operation
    for (auto operand : op->getOperands()) {
      llvm::outs() << "  Operand: " << operand << "\n";
    }
    for (auto attr : op->getAttrs()) {
      llvm::outs() << "  Attribute: " << attr.getName() << " = " << attr.getValue() << "\n";
    }
    // Perform specific analysis for load operations (e.g., dependency analysis)
    // ...
  }

  void analyzeAdd(Operation *op) {
    llvm::outs() << "Analyzing affrec.add operation:\n";
    // Access operands and attributes of the add operation
    for (auto operand : op->getOperands()) {
      llvm::outs() << "  Operand: " << operand << "\n";
    }
    for (auto attr : op->getAttrs()) {
      llvm::outs() << "  Attribute: " << attr.getName() << " = " << attr.getValue() << "\n";
    }
    // Perform specific analysis for add operations (e.g., dependency analysis)
    // ...
  }

  void analyzeStore(Operation *op) {
    llvm::outs() << "Analyzing affrec.store operation:\n";
    // Access operands and attributes of the store operation
    for (auto operand : op->getOperands()) {
      llvm::outs() << "  Operand: " << operand << "\n";
    }
    for (auto attr : op->getAttrs()) {
      llvm::outs() << "  Attribute: " << attr.getName() << " = " << attr.getValue() << "\n";
    }
    // Perform specific analysis for store operations (e.g., dependency analysis)
    // ...
  }
};

} // namespace

namespace mlir {
namespace affrec {

void registerAnalysisPasses() {
  PassRegistration<AffRecAnalysisPass>("my-analysis-pass", "Analyze affrec dialect operations");
}

} // namespace affrec
} // namespace mlir

// Example of how to register the pass, would be located in a .cpp file that gets linked into your mlir-opt.
extern "C" void mlirCreateDialectPasses() {
  mlir::affrec::registerAnalysisPasses();
}
```

**Explanation:**

-  **Headers:**
    * Include necessary MLIR headers for working with operations, passes, and dialects.
    * Include LLVM headers for output and data structures.

-  **`AffRecAnalysisPass` Struct:**
    * This struct defines your analysis pass, inheriting from `PassWrapper`.
    * `runOnOperation()`: This is the main function of the pass, which is called for each function in the MLIR module.
        * It uses `func.walk()` to traverse all operations within the function.
        * It checks the operation name and calls specific analysis functions (e.g., `analyzeLoad`, `analyzeAdd`, `analyzeStore`).

-  **Analysis Functions (`analyzeLoad`, `analyzeAdd`, `analyzeStore`):**
    * These functions perform the actual analysis of the `affrec` dialect operations.
    * They use `op->getOperands()` and `op->getAttrs()` to access the operands and attributes of the operations.
    * Currently, the provided code only prints the operands and attributes. You will want to replace the `...` comments with your desired analysis.
    * Example analysis that you would implement:
        * Parsing the affine expressions within the attributes.
        * Building a dependency graph based on the affine expressions.
        * Detecting data dependencies between operations.

-  **Registration:**
    * The `registerAnalysisPasses()` function registers your pass with the MLIR pass manager.
    * The `PassRegistration` template registers the pass with a unique name (`"my-analysis-pass"`) and a description.
    * The `mlirCreateDialectPasses()` function is an extern "C" function, which is needed so that mlir-opt can find and use the registered passes.

**How to Build and Run:**

    -  **Save:** Save the code as `my-analysis-pass.cpp`.
    -  **Build:** You'll need to compile and link this code with your MLIR build. This typically involves modifying your MLIR build system (CMakeLists.txt) to include your pass.
        * Add your cpp file to the correct target, which will depend on how your MLIR project is structured. If you have a local build, you will likely need to create a new target, and link against the MLIR libraries.
    -  **Run:** Once you've built your pass, you can run `mlir-opt` with the `-my-analysis-pass` option:

```bash
mlir-opt input.mlir -my-analysis-pass
```

**Important Notes:**

    * **Replace `...` with your analysis logic:** The provided code is a starting point. You'll need to implement the specific analysis you want to perform.
    * **Dependency Analysis:** When implementing dependency analysis, you'll likely need to use MLIR's dataflow analysis APIs to track the flow of values between operations.
    * **Affine Expressions:** For working with affine expressions, you can use MLIR's affine dialect and its associated APIs.
    * **Error Handling:** Add error handling to your pass to gracefully handle invalid MLIR code or unexpected situations.
    * **Testing:** Write unit tests for your pass to ensure its correctness.
    * **CMakeLists.txt:** The most difficult part of adding a pass is adding it to the CMake build system. You will want to look at other examples in the MLIR source tree to see how other passes are added.


Step 3.  **Build your pass:**

```bash
# ... (build commands to compile and link your pass) ...
```

Step 4.  **Run `mlir-opt`:**

```bash
mlir-opt input.mlir -my-analysis-pass
```

By using `mlir-opt` in this way, you can gradually build and refine your analysis passes and integrate them into your DL compiler pipeline.

