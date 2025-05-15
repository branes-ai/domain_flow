#pragma once
#include <variant>
#include <map>
#include <string>
#include <energy/event_type.hpp>

namespace sw::energy {

    // Compute events are arithmetic/logic/function transformations
    struct ComputeEventKey {
        EventType op;
        uint32_t width;

        bool operator<(const ComputeEventKey& other) const {
            return std::tie(op, width) < std::tie(other.op, other.width);
        }
    };

    // Memory events are reads/writes to register files, caches, and dram
    struct MemoryEventKey {
        EventType op;
        uint32_t width;
        uint8_t burstLength;

        bool operator<(const MemoryEventKey& other) const {
            return std::tie(op, width, burstLength) < std::tie(other.op, other.width, other.burstLength);
        }
    };

    // Network events are packet reads/writes on a bus or hop/forward network infrastructure
    struct NetworkEventKey {
        EventType op;
        uint32_t width;
        uint8_t burstLength;

        bool operator<(const NetworkEventKey& other) const {
            return std::tie(op, width, burstLength) < std::tie(other.op, other.width, other.burstLength);
        }
    };

    class EventCounterDatabase {
    private:
        std::map<ComputeEventKey, uint64_t> computeEventCounts;
        std::map<MemoryEventKey, uint64_t> memoryEventCounts;
        std::map<NetworkEventKey, uint64_t> networkEventCounts;

    public:
        void increment(const ComputeEvent& event, uint64_t count = 1) {
            computeEventCounts[{event.op, event.width}] += count;
        }

        void increment(const MemoryEvent& event, uint64_t count = 1) {
            memoryEventCounts[{event.op, event.width, event.burstLength}] += count;
        }

        void increment(const NetworkEvent& event, uint64_t count = 1) {
            networkEventCounts[{event.op, event.width, event.burstLength}] += count;
        }

        uint64_t getCount(const ComputeEvent& event) const {
            auto it = computeEventCounts.find({ event.op, event.width });
            return (it != computeEventCounts.end()) ? it->second : 0;
        }

        uint64_t getCount(const MemoryEvent& event) const {
            auto it = memoryEventCounts.find({ event.op, event.width, event.burstLength });
            return (it != memoryEventCounts.end()) ? it->second : 0;
        }

        uint64_t getCount(const NetworkEvent& event) const {
            auto it = networkEventCounts.find({ event.op, event.width, event.burstLength });
            return (it != networkEventCounts.end()) ? it->second : 0;
        }

        const std::map<ComputeEventKey, uint64_t>& getComputeEventCounts() const {
            return computeEventCounts;
        }

        const std::map<MemoryEventKey, uint64_t>& getMemoryEventCounts() const {
            return memoryEventCounts;
        }

        const std::map<NetworkEventKey, uint64_t>& getNetworkEventCounts() const {
            return networkEventCounts;
        }

        // ... methods to iterate through counts, clear counters, etc.
    };

}