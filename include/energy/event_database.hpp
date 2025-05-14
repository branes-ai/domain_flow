#pragma once
#include <variant>
#include <vector>
#include <string>
#include <energy/event_type.hpp>

namespace sw::energy {

    struct ComputationalEvent {
        EventType type;
        EventData data;
        // ... common event metadata (timestamp, core ID, etc.)
    };

    class EventDatabase {
    private:
        std::vector<ComputationalEvent> events;

    public:
        void addEvent(const ComputationalEvent& event) {
            events.push_back(event);
        }

        const std::vector<ComputationalEvent>& getEvents() const {
            return events;
        }

        // ... methods to query and process events (e.g., filter by type)
    };

}