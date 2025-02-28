# IREE Configurations

IREE HAL drivers:
  - local-sync
  - local-task
  - vulkan

IREE HAL local executable library loaders:
  - embedded-elf
  - system-library
  - vmvx-module

IREE HAL local executable plugin mechanisms:
  - embedded-elf
  - system-library

IREE compiler input dialects:
  - StableHLO
  - Torch MLIR
  - TOSA

IREE compiler output formats:
  - 'vm-c': textual C source module
  - 'vm-bytecode': VM bytecode
  - 'vm-asm': VM MLIR assembly