# IREE Virtual Machine

The VM is an abstract machine that defines a type system of primitive and reference objects, module machinery, and a fairly involved mechanic for calls and dynamically binding extern funcs to their defs.

It comes with a bytecode module type, which implements the module interface and exposes a CFG based instruction set (sans pointers, as there are some security and portability to device scheduler goals in play) with an infinite number of registers. A VMFB, which the compiler produces by default is a serialization of this. The bytecode module has an interpreter for it.

## VM modules

There are multiple module types provided by default in the runtime:

 1. HAL for interacting with devices
 2. **io_params** for external data
 3. check for testing

There are also Python bindings to create a module object dynamically and define exports in Python. Since the module interface is just natively defined in C, IREE also lets you load a .so dynamically which exports a function to create a new named module (this is what emitc produces for example).

When the tools are creating an **iree_vm_context** for execution, this is primarily about instantiating modules and adding them. Each module resolved its imports from the exports of the priors. In practice, an **io_params** module is added to access parameter providers, a hal module for devices, one bytecode module for each vmfb listed, and a native .so module for any .so listed.

That's all it is at the runtime. There's a lot of machinery on the compiler side for producing these modules and their interplay. The lowest level there to get a feel for what the compiler can emit, either a bytecode or a C based export, look at the vm dialect.

## Lowering from the VM to C

There has been talk for years of having a direct lowering of VM to LLVM without going through C. While desirable in theory, it's just never become a priority... The C based export is what embedded folks want (because you never want to pin your system toolchain to a random LLVM like that). And the bytecode side has never been the bottleneck for others. It's also an annoying, maintenance prone bit of code to write and just never got done.

The high "it just works" quotient on the bytecode side has probably helped drive that too. "vmfb" has become a bit synonymous with "IREE" and teams using it think that is the main thing. But it is just one serialization of a VM module defining a program... But it has the best tools and debugging experience.

## Call interface

The VM call interface is modeled as a coroutine, and the bytecode interpreter supports multi task suspend/resume on a single thread. This is used for a lot of things (i.e. multi batch, async submissive, interfacing to io fabric, etc). Most of what people think of as "async" in the context of device interactions comes down to this and the cooperation of the hal module which provides device based synchronization and scheduling primitives.

The way it is structured, the VM was not designed to block, but it can suspend.

# IREE VMFB Serialization

VMFB (Virtual Machine FlatBuffer) is IREE's serialization format for compiled modules, and it's heavily used in IREE's Vulkan backend.

Here's a breakdown of where to find the relevant source code in the `iree-dev` tree, along with some context:

**Key Areas:**

1.  **FlatBuffer Schemas (`iree/schemas/`)**:
    * The core definition of the VMFB format is in the FlatBuffer schemas. These schemas define the structure of the serialized data. Look for files like `module_def.fbs` within this directory. This is where the structure of the module, functions, and other components are defined.
    * This is the absolute first place to look to understand the structure of the vmfb.

2.  **Serialization/Deserialization Code (`iree/compiler/` and `iree/runtime/`)**:
    * **Compiler (`iree/compiler/`)**:
        * The compiler is responsible for generating the VMFB. You'll find code that takes the IREE's internal representation of a module and serializes it into the VMFB format.
        * Look in `iree/compiler/Dialect/HAL/Target/Vulkan/` for vulkan specific serialization.
        * Also look in `iree/compiler/Dialect/HAL/Target/` for general hal target serialization.
    * **Runtime (`iree/runtime/`)**:
        * The runtime is responsible for loading and executing VMFBs. You'll find code that parses the VMFB format and reconstructs the module in memory.
        * Look in `iree/runtime/src/iree/hal/drivers/vulkan/` for vulkan specific vmfb loading.
        * Look in `iree/runtime/src/iree/hal/drivers/` for general hal driver vmfb loading.
    * The hal drivers are the main area that loads vmfb files.

3.  **Vulkan-Specific Code (`iree/runtime/src/iree/hal/drivers/vulkan/`)**:
    * This directory contains the Vulkan HAL driver, which is responsible for executing VMFBs on Vulkan devices. You'll find code that handles the Vulkan-specific aspects of loading and executing the compiled modules.

**Confirmation/Correction:**

* Yes, VMFB is used for IREE's Vulkan backend.
* It's more accurate to describe VMFB as a general-purpose serialization format for IREE modules, not strictly "Vulkan-like." While it's heavily used in the Vulkan backend, it's also used for other backends.
* The VMFB contains the compiled code, which could be vulkan shader bytecode, or other target specific bytecode.
* VMFB is a flatbuffer, which is a very efficient way to serialize data.

**How to Explore:**

1.  **Start with the Schemas:** Begin by examining the FlatBuffer schemas in `iree/schemas/`. This will give you a high-level understanding of the VMFB format.
2.  **Trace the Compilation Pipeline:** Follow the compilation pipeline in `iree/compiler/` to see how the VMFB is generated.
3.  **Trace the Runtime Loading:** Follow the runtime loading process in `iree/runtime/` to see how the VMFB is parsed and executed.
4.  **Use Code Search:** Use a code search tool (like `grep` or your IDE's search functionality) to search for keywords like "VMFB," "FlatBuffer," and the names of the FlatBuffer tables and fields.

By exploring these areas, you'll gain a deep understanding of how VMFBs are serialized and deserialized in IREE.
`
## Virtual Machine FlatBuffer == VMFB

IREE's module serialization format is "VMFB".

Here's a breakdown of what VMFB is:

* **VMFB is primarily a serialization format:**
    * It's designed to efficiently store and load compiled IREE modules.
    * This means it holds the results of IREE's compilation process, which can include various forms of compiled code, such as:
        * Machine code for CPUs
        * Shader bytecode for GPUs (e.g., SPIR-V for Vulkan)
        * Other target-specific instructions
* **It's not strictly a "graph format" in the sense of directly representing a high-level model graph:**
    * While the compiled module within a VMFB originates from a model graph, the VMFB itself contains the lowered and compiled results of that graph.
    * It is more accurate to say that it contains the results of the compiled graph.
* **It has elements of a "virtual machine format":**
    * The VMFB can contain bytecode that is executed by IREE's runtime, which acts as a lightweight virtual machine.
    * So in that respect it does contain elements of a virtual machine format.
* **Key takeaway:**
    * VMFB is a container for compiled code and data, optimized for efficient deployment.

In summary, VMFB serves as IREE's deployment artifact, holding the compiled results of machine learning models. It contains the instructions and data needed for IREE's runtime to execute those models on target hardware.

## File format information: tflite -> iree-import -> mlirbc -> compile -> vmfb -> runtime

It's important to understand the direction of the IREE compilation flow to answer this question accurately. Here's a breakdown:

**IREE Compilation Flow:**

* **Input:** Machine learning models from various frameworks (TensorFlow, PyTorch, etc.).
* **MLIR Representation:** These models are converted into MLIR's intermediate representation. This is where the "MLIR graph" exists.
* **Optimization and Lowering:** IREE's compiler performs various optimizations and lowers the MLIR representation to more hardware-specific instructions.
* **Target-Specific Code Generation:** The compiler generates target-specific code (e.g., machine code, SPIR-V).
* **VMFB Serialization:** The resulting compiled module, including the target-specific code and other necessary data, is serialized into the VMFB format.
* **Runtime Execution:** The IREE runtime loads and executes the VMFB.

**Going Back from VMFB to MLIR:**

* It's generally not a straightforward, lossless process to perfectly reconstruct the original MLIR graph from a VMFB file.
* Here's why:
    * **Loss of Information:** The compilation process involves optimizations and lowerings that can transform and remove information from the original MLIR graph. For example, some high-level operations might be fused or replaced with lower-level instructions.
    * **Target-Specific Code:** The VMFB contains target-specific code, which is a very low-level representation. Reconstructing the higher-level MLIR operations from this code is a complex task.
    * **Focus on Execution:** The VMFB is designed for efficient execution, not for reverse engineering the original model.

* **What is possible:**
    * It's possible to extract certain information from a VMFB file, such as the compiled code and some metadata.
    * It is possible to get information regarding the operations that are being executed, but to recreate the exact MLIR that created that VMFB is very difficult.
    * Tools exist that allow you to inspect the contents of VMFB files, and to extract the contained binary files. This can be very useful for debugging.

**In summary:**

* While you can extract information from a VMFB, reconstructing the exact original MLIR graph is generally not feasible due to the loss of information during compilation.
* The VMFB is an end product of the compilation pipeline, designed for efficient runtime execution.

