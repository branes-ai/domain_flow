# IREE Serialization

The `iree-import` and `iree-compile` tools are central to IREE's compilation workflow, and they handle MLIR serialization and deserialization. However, the code is spread across various files and directories due to IREE's modular design.

Here's a breakdown of where to find the relevant code in the IREE GitHub repository:

**1. `iree-import` (MLIR Serialization):**

* **Core MLIR Output:**
    * The `iree-import` tool primarily focuses on converting models from various frameworks (TensorFlow, PyTorch, etc.) into IREE's MLIR dialect.
    * The actual MLIR serialization (writing MLIR to a file) is handled by MLIR's built-in serialization mechanisms.
    * You'll find the driver code for `iree-import` in `iree/compiler/src/iree-import/iree-import.cc`.
    * The main function in this file sets up the MLIR context, parses command-line arguments, imports the model, and then uses MLIR's `print` functions to write the MLIR to a file or standard output.

* **MLIR's Serialization:**
    * The core MLIR serialization logic resides within the MLIR library itself.
    * Look into the MLIR source code, especially in the `mlir/lib/Parser/` and `mlir/lib/IR/` directories, for the implementation of `print` and other serialization functions.

**2. `iree-compile` (MLIR Deserialization and Compilation):**

* **MLIR Parsing:**
    * The `iree-compile` tool reads MLIR from a file or standard input.
    * This is done using MLIR's parser, which is also part of the MLIR library.
    * The main driver is located at `iree/compiler/src/iree-compile/iree-compile.cc`
    * The parsing is done using the MLIR source manager and parse functions.

* **IREE Compiler Pipeline:**
    * After parsing the MLIR, `iree-compile` runs it through IREE's compiler pipeline.
    * This pipeline involves various passes that optimize and lower the MLIR to target-specific code.
    * The compiler pipeline setup and execution are handled by code in `iree/compiler/src/iree-compile/iree-compile.cc` and the related compiler libraries.
    * The hal target specific lowering, and vmfb generation happens in `iree/compiler/Dialect/HAL/Target/`

* **VMFB Generation:**
    * The VMFB generation happens at the end of the compilation pipeline. The hal target specific files are where the flatbuffer is built.

**Key Points:**

* IREE heavily relies on MLIR's built-in serialization and parsing capabilities.
* The `iree-import` and `iree-compile` tools provide the command-line interface and orchestrate the compilation process.
* The VMFB creation is done during the hal target lowering process.
* To find the exact MLIR serialization/deserialization code, you'll need to delve into the MLIR library's source code.

