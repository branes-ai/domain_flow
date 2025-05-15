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

    std::ostream& operator<<(std::ostream& os, EventType type) {
        switch (type) {
        case EventType::ALU_ADD: os << "ALU_ADD"; break;
        case EventType::ALU_SUB: os << "ALU_SUB"; break;
        case EventType::ALU_MUL: os << "ALU_MUL"; break;
        case EventType::ALU_DIV: os << "ALU_DIV"; break;
        case EventType::ALU_MOD: os << "ALU_MOD"; break;
        case EventType::ALU_FADD: os << "ALU_FADD"; break;
        case EventType::ALU_FSUB: os << "ALU_FSUB"; break;
        case EventType::ALU_FMUL: os << "ALU_FMUL"; break;
        case EventType::ALU_FDIV: os << "ALU_FDIV"; break;
        case EventType::ALU_FMA: os << "ALU_FMA"; break;
        case EventType::ALU_SFU: os << "ALU_SFU"; break;
        case EventType::RF_READ: os << "RF_READ"; break;
        case EventType::RF_WRITE: os << "RF_WRITE"; break;
        case EventType::VRF_READ: os << "VRF_READ"; break;
        case EventType::VRF_WRITE: os << "VRF_WRITE"; break;
        case EventType::L1_CACHE_READ: os << "L1_CACHE_READ"; break;
        case EventType::L1_CACHE_WRITE: os << "L1_CACHE_WRITE"; break;
        case EventType::L2_CACHE_READ: os << "L2_CACHE_READ"; break;
        case EventType::L2_CACHE_WRITE: os << "L2_CACHE_WRITE"; break;
        case EventType::L3_CACHE_READ: os << "L3_CACHE_READ"; break;
        case EventType::L3_CACHE_WRITE: os << "L3_CACHE_WRITE"; break;
        case EventType::DRAM_READ: os << "DRAM_READ"; break;
        case EventType::DRAM_WRITE: os << "DRAM_WRITE"; break;
        case EventType::BUS_READ: os << "BUS_READ"; break;
        case EventType::BUS_WRITE: os << "BUS_WRITE"; break;
        case EventType::NETWORK_READ: os << "NETWORK_READ"; break;
        case EventType::NETWORK_WRITE: os << "NETWORK_WRITE"; break;
        case EventType::L1_CAM_READ: os << "L1_CAM_READ"; break;
        case EventType::L1_CAM_WRITE: os << "L1_CAM_WRITE"; break;
        case EventType::L2_CAM_READ: os << "L2_CAM_READ"; break;
        case EventType::L2_CAM_WRITE: os << "L2_CAM_WRITE"; break;
        case EventType::TLB_READ: os << "TLB_READ"; break;
        case EventType::TLB_WRITE: os << "TLB_WRITE"; break;
        case EventType::DIRECTORY_READ: os << "DIRECTORY_READ"; break;
        case EventType::DIRECTORY_WRITE: os << "DIRECTORY_WRITE"; break;
        default: os << static_cast<int>(type); break;
        }
        return os;
    }

    enum class AccessType { READ, WRITE };

    // Compute events are arithmetic/logic/function transformations
    struct ComputeEvent { 
        EventType op;
        uint32_t width;
    };

    // Memory events are reads/writes to register files, caches, and dram
    struct MemoryEvent { 
        EventType op; 
        uint32_t width;
        uint8_t burstLength; // N/A if there are no bursts
    };

    // Network events are packet reads/writes on a bus or hop/forward network infrastructure
    struct NetworkEvent {
        EventType op; 
        uint32_t width;
        uint8_t burstLength; // N/A if there are no bursts
    };

}
