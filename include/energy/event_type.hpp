#pragma once
#include <variant>
#include <vector>
#include <string>

namespace sw::energy {

    enum class EventType {
        INSTR_FETCH,
        INSTR_DECODE,
        INSTR_DISPATCH,
        ALU_IADD,
        ALU_ISUB,
        ALU_IMUL,
        ALU_IDIV,
        ALU_IMOD,
        ALU_ISFU,
        ALU_FADD,
        ALU_FSUB,
        ALU_FMUL,
        ALU_FDIV,
        ALU_FMA,
        ALU_FSFU,
        WRITE_BACK,
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
        case EventType::INSTR_FETCH: os << "INSTR_FETCH"; break;
        case EventType::INSTR_DECODE: os << "INSTR_DECODE"; break;
        case EventType::INSTR_DISPATCH: os << "INSTR_DISPATCH"; break;
        case EventType::ALU_IADD: os << "ALU_IADD"; break;
        case EventType::ALU_ISUB: os << "ALU_ISUB"; break;
        case EventType::ALU_IMUL: os << "ALU_IMUL"; break;
        case EventType::ALU_IDIV: os << "ALU_IDIV"; break;
        case EventType::ALU_IMOD: os << "ALU_IMOD"; break;
        case EventType::ALU_ISFU: os << "ALU_ISFU"; break;
        case EventType::ALU_FADD: os << "ALU_FADD"; break;
        case EventType::ALU_FSUB: os << "ALU_FSUB"; break;
        case EventType::ALU_FMUL: os << "ALU_FMUL"; break;
        case EventType::ALU_FDIV: os << "ALU_FDIV"; break;
        case EventType::ALU_FMA: os << "ALU_FMA"; break;
        case EventType::ALU_FSFU: os << "ALU_FSFU"; break;
        case EventType::WRITE_BACK: os << "WRITE_BACK"; break;
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

    inline std::istream& operator>>(std::istream& is, EventType& type) {
        std::string typeStr;
        is >> typeStr;
        if (typeStr == "INSTR_FETCH") {
            type = EventType::INSTR_FETCH;
        }
        else if (typeStr == "INSTR_DECODE") {
            type = EventType::INSTR_DECODE;
        }
        else if (typeStr == "INSTR_DISPATCH") {
            type = EventType::INSTR_DISPATCH;
        }
        else if (typeStr == "ALU_IADD") {
            type = EventType::ALU_IADD;
        }
        else if (typeStr == "ALU_ISUB") {
            type = EventType::ALU_ISUB;
        }
        else if (typeStr == "ALUI_MUL") {
            type = EventType::ALU_IMUL;
        }
        else if (typeStr == "ALU_IDIV") {
            type = EventType::ALU_IDIV;
        }
        else if (typeStr == "ALU_IMOD") {
            type = EventType::ALU_IMOD;
        }
        else if (typeStr == "ALU_ISFU") {
            type = EventType::ALU_ISFU;
        }
        else if (typeStr == "ALU_FADD") {
            type = EventType::ALU_FADD;
        }
        else if (typeStr == "ALU_FSUB") {
            type = EventType::ALU_FSUB;
        }
        else if (typeStr == "ALU_FMUL") {
            type = EventType::ALU_FMUL;
        }
        else if (typeStr == "ALU_FDIV") {
            type = EventType::ALU_FDIV;
        }
        else if (typeStr == "ALU_FMA") {
            type = EventType::ALU_FMA;
        }
        else if (typeStr == "ALU_FSFU") {
            type = EventType::ALU_FSFU;
        }
        else if (typeStr == "WRITE_BACK") {
            type = EventType::WRITE_BACK;
        }
        else if (typeStr == "RF_READ") {
            type = EventType::RF_READ;
        }
        else if (typeStr == "RF_WRITE") {
            type = EventType::RF_WRITE;
        }
        else if (typeStr == "VRF_READ") {
            type = EventType::VRF_READ;
        }
        else if (typeStr == "VRF_WRITE") {
            type = EventType::VRF_WRITE;
        }
        else if (typeStr == "L1_CACHE_READ") {
            type = EventType::L1_CACHE_READ;
        }
        else if (typeStr == "L1_CACHE_WRITE") {
            type = EventType::L1_CACHE_WRITE;
        }
        else if (typeStr == "L2_CACHE_READ") {
            type = EventType::L2_CACHE_READ;
        }
        else if (typeStr == "L2_CACHE_WRITE") {
            type = EventType::L2_CACHE_WRITE;
        }
        else if (typeStr == "L3_CACHE_READ") {
            type = EventType::L3_CACHE_READ;
        }
        else if (typeStr == "L3_CACHE_WRITE") {
            type = EventType::L3_CACHE_WRITE;
        }
        else if (typeStr == "DRAM_READ") {
            type = EventType::DRAM_READ;
        }
        else if (typeStr == "DRAM_WRITE") {
            type = EventType::DRAM_WRITE;
        }
        else if (typeStr == "BUS_READ") {
            type = EventType::BUS_READ;
        }
        else if (typeStr == "BUS_WRITE") {
            type = EventType::BUS_WRITE;
        }
        else if (typeStr == "NETWORK_READ") {
            type = EventType::NETWORK_READ;
        }
        else if (typeStr == "NETWORK_WRITE") {
            type = EventType::NETWORK_WRITE;
        }
        else if (typeStr == "L1_CAM_READ") {
            type = EventType::L1_CAM_READ;
        }
        else if (typeStr == "L1_CAM_WRITE") {
            type = EventType::L1_CAM_WRITE;
        }
        else if (typeStr == "L2_CAM_READ") {
            type = EventType::L2_CAM_READ;
        }
        else if (typeStr == "L2_CAM_WRITE") {
            type = EventType::L2_CAM_WRITE;
        }
        else if (typeStr == "TLB_READ") {
            type = EventType::TLB_READ;
        }
        else if (typeStr == "TLB_WRITE") {
            type = EventType::TLB_WRITE;
        }
        else if (typeStr == "DIRECTORY_READ") {
            type = EventType::DIRECTORY_READ;
        }
        else if (typeStr == "DIRECTORY_WRITE") {
            type = EventType::DIRECTORY_WRITE;
        }
        else {
            is.setstate(std::ios::failbit); // Indicate that the input was not a valid EventType
        }
        return is;
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
