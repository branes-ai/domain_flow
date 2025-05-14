#pragma once
#include <variant>
#include <vector>
#include <string>

namespace sw::energy {

    enum class EventType {
        ALU_INTEGER_ADD,
        REGISTER_READ,
        CACHE_ACCESS,
        DRAM_ACCESS,
        BUS_ACCESS
    };

    enum class IntegerSizeType { BITS_4, BITS_8, BITS_16, BITS_32, BITS_64 };
    enum class RegisterSizeType { BITS_8, BITS_16, BITS_32, BITS_64, BITS_128, BITS_256 };
    enum class CacheSizeType { BITS_128, BITS_256, BITS_512, BITS_1024 };

    struct ALUIntegerAddEvent { IntegerSizeType size; /* ... other ALU details */ };
    struct RegisterReadEvent { RegisterSizeType size; unsigned int address; /* ... */ };
    struct CacheAccessEvent { enum class AccessType { READ, WRITE }; AccessType type; CacheSizeType size; unsigned int address; /* ... */ };
    struct DRAMAccessEvent { enum class AccessType { READ, WRITE }; AccessType type; int burstCount; /* ... */ };
    struct BusAccessEvent { enum class AccessType { READ, WRITE }; AccessType type; std::string busType; int burstLength; /* ... */ };

    using EventData = std::variant<
        ALUIntegerAddEvent,
        RegisterReadEvent,
        CacheAccessEvent,
        DRAMAccessEvent,
        BusAccessEvent
    >;

}
