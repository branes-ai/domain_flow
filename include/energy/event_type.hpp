#pragma once
#include <variant>
#include <vector>
#include <string>

namespace sw::energy {

    enum class EventType {
        ALU_ADD,
        ALU_SUB,
        ALU_MUL,
        ALU_DIV,
        ALU_MOD,
        ALU_FADD,
        ALU_FSUB,
        ALU_FMUL,
        ALU_FDIV,
        ALU_FMA,
        ALU_SFU,
        RF_READ,
        RF_WRITE,
        VRF_READ,
        VRF_WRITE,
        L1_CACHE_READ,
        L1_CACHE_WRITE,
        L2_CACHE_READ,
        L2_CACHE_WRITE,
        L3_CACHE_READ,
        L3_CACHE_WRITE,
        DRAM_READ,
        DRAM_WRITE,
        BUS_READ,
        BUS_WRITE,
        NETWORK_READ,
        NETWORK_WRITE,
        L1_CAM_READ,
        L1_CAM_WRITE,
        L2_CAM_READ,
        L2_CAM_WRITE,
        TLB_READ,
        TLB_WRITE,
        DIRECTORY_READ,
        DIRECTORY_WRITE
    };

    enum class AccessType { READ, WRITE };

    // Compute events are arithmetic/logic/function transformations
    struct ComputeEvent { 
        EventType op;
        uint8_t width;
    };

    // Memory events are reads/writes to register files, caches, and dram
    struct MemoryEvent { 
        EventType op; 
        uint8_t width;
        uint8_t burstLength; // N/A if there are no bursts
    };

    // Network events are packet reads/writes on a bus or hop/forward network infrastructure
    struct NetworkEvent {
        EventType op; 
        uint8_t width;
        uint8_t burstLength; // N/A if there are no bursts
    };

}
