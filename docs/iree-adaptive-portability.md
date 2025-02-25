# IREE portability 

Question: I just used the IREE compiler using a vulkan device target to generate vmfbs for AMD gfx1101 and an NVIDIA sm_86 and now I am running these through iree-run-module on an x86 laptop without these GPUs, and it produces results fine. So that is going on here?

You've hit upon a key aspect of IREE's design and its ability to target diverse hardware.  Here's what's happening:

1. **Compilation Targets, Not Execution Targets:** When you compile with IREE, you're targeting a *compilation target*, not necessarily the *execution target*.  You're telling the compiler *how* to optimize the code, not *where* it will ultimately run.

2. **Vulkan as an Intermediate:** Vulkan is not just a GPU API; it's also used by IREE as an *intermediate representation*.  IREE can compile *to* Vulkan, even if you don't have a Vulkan-capable GPU at runtime.  The Vulkan bytecode (SPIR-V) that IREE generates can be further processed or interpreted.

3. **CPU Fallback:**  IREE has a CPU execution backend.  When you run the compiled module on your x86 laptop, even though it was compiled targeting Vulkan for specific GPUs, the IREE runtime can detect that those GPUs aren't available. It will then fall back to the CPU execution backend.

4. **SPIR-V Interpretation/Translation:**  The SPIR-V generated for the Vulkan target can be interpreted or translated to CPU instructions at runtime.  IREE's CPU backend is capable of handling this.  It might not be as performant as running on the intended GPU, but it allows for execution on the CPU.

5. **Portable Bytecode:**  The `.vmfb` file contains portable bytecode (often containing SPIR-V or an intermediate representation derived from it). This bytecode is not tied to a specific GPU architecture in the sense that it *must* run on that architecture.  IREE's runtime can take this portable bytecode and adapt it to the available hardware.

**In simpler terms:**

Imagine you have a recipe for a cake that's optimized for a specific oven. You can still bake that cake in a different oven; it just might not be *perfectly* optimized for that second oven.  IREE's compilation is like creating the recipe.  The `.vmfb` is the recipe.  You can then "bake" (run) that recipe (module) on different "ovens" (hardware), even if the recipe was initially optimized for a specific one.  The IREE runtime acts as the "chef" who can adapt the recipe to the available oven.

**Why is this useful?**

* **Offline Compilation:** You can compile your models ahead of time, targeting specific hardware, and then deploy them to devices that might not have the same hardware.
* **Flexibility:** You can run models on a range of devices without needing to recompile for each one.
* **Debugging:** You can develop and debug on a CPU even if your target deployment is a GPU.

**Important Note:** While IREE can fall back to CPU execution, the performance will usually be significantly lower than running on the targeted GPU.  The compilation is optimized for the specific GPU architecture, and the CPU fallback is a more general-purpose solution.  If you have the target GPU available, that will almost always be the best option for performance.
