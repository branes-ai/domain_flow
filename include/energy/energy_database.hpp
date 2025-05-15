#pragma once
#include <map>
#include <string>
#include <variant>
#include <energy/event_type.hpp>
#include <energy/event_database.hpp>

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
        std::map<EventType, std::map<std::variant<uint8_t, std::string>, EnergyValue>> energyMap;
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

        std::optional<EnergyValue> getEnergyValue(Technology technology, EventType eventType, const std::variant<uint8_t, std::string>& parameter) const {
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

        // Overload for ComputeEventKey
        std::optional<EnergyValue> getEnergyValue(Technology technology, const ComputeEventKey& key) const {
            auto techData = getTechnologyData(technology);
            if (techData) {
                auto eventIt = techData->energyMap.find(key.op);
                if (eventIt != techData->energyMap.end()) {
                    auto paramIt = eventIt->second.find(key.size);
                    if (paramIt != eventIt->second.end()) {
                        return paramIt->second;
                    }
                }
            }
            return std::nullopt;
        }

        // Overload for MemoryEventKey and NetworkEventKey (assuming width and burst are the relevant parameters)
        std::optional<EnergyValue> getEnergyValue(Technology technology, const MemoryEventKey& key) const {
            auto techData = getTechnologyData(technology);
            if (techData) {
                auto eventIt = techData->energyMap.find(key.op);
                if (eventIt != techData->energyMap.end()) {
                    // You might need a more specific key in your energy map if width and burst are distinct
                    std::string combinedKey = std::to_string(key.width) + "_" + std::to_string(key.burstLength);
                    auto paramIt = eventIt->second.find(combinedKey);
                    if (paramIt != eventIt->second.end()) {
                        return paramIt->second;
                    }
                    // Alternatively, if width is the primary key:
                    auto widthIt = eventIt->second.find(key.width);
                    if (widthIt != eventIt->second.end()) {
                        return widthIt->second;
                    }
                    // And potentially handle burst length separately if needed in your energy model
                }
            }
            return std::nullopt;
        }

        std::optional<EnergyValue> getEnergyValue(Technology technology, const NetworkEventKey& key) const {
            // Similar logic as MemoryEventKey, adjust based on how your energy map is structured for network events
            auto techData = getTechnologyData(technology);
            if (techData) {
                auto eventIt = techData->energyMap.find(key.op);
                if (eventIt != techData->energyMap.end()) {
                    std::string combinedKey = std::to_string(key.width) + "_" + std::to_string(key.burstLength);
                    auto paramIt = eventIt->second.find(combinedKey);
                    if (paramIt != eventIt->second.end()) {
                        return paramIt->second;
                    }
                    auto widthIt = eventIt->second.find(key.width);
                    if (widthIt != eventIt->second.end()) {
                        return widthIt->second;
                    }
                }
            }
            return std::nullopt;
        }
    };

}