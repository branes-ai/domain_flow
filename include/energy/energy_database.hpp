#pragma once
#include <map>
#include <string>
#include <variant>
#include <energy/event_type.hpp>

namespace sw::energy {

    enum class Technology {
        TSMC_5NM,
        INTEL_18A,
        // ... other technologies
    };

    struct PerBitEnergy { double joulesPerBit; };
    struct PerBurstEnergy { double joulesPerBurst; };
    struct FixedEnergy { double joules; };

    using EnergyValue = std::variant<PerBitEnergy, PerBurstEnergy, FixedEnergy>;

    struct TechnologyEnergyData {
        Technology technology;
        std::map<EventType, std::map<std::variant<IntegerSizeType, RegisterSizeType, CacheSizeType, std::string /*bus type*/ >, EnergyValue>> energyMap;
    };

    class EnergyDatabase {
    private:
        std::map<Technology, TechnologyEnergyData> technologyEnergies;

    public:
        void addTechnologyData(const TechnologyEnergyData& data) {
            technologyEnergies[data.technology] = data;
        }

        const TechnologyEnergyData* getTechnologyData(Technology technology) const {
            auto it = technologyEnergies.find(technology);
            if (it != technologyEnergies.end()) {
                return &it->second;
            }
            return nullptr;
        }

        // Method to retrieve energy value for a specific event type and size/parameter
        std::optional<EnergyValue> getEnergyValue(Technology technology, EventType eventType, const std::variant<IntegerSizeType, RegisterSizeType, CacheSizeType, std::string>& parameter) const {
            auto techData = getTechnologyData(technology);
            if (techData) {
                auto eventIt = techData->energyMap.find(eventType);
                if (eventIt != techData->energyMap.end()) {
                    auto paramIt = eventIt->second.find(parameter);
                    if (paramIt != eventIt->second.end()) {
                        return paramIt->second;
                    }
                }
            }
            return std::nullopt;
        }
    };

}