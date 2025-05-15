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

    inline std::ostream& operator<<(std::ostream& ostr, const Technology& tech) {
        switch (tech) {
        case Technology::TSMC_5NM:
            ostr << "Technology::TSMC_5NM";
            break;
        case Technology::INTEL_18A:
            ostr << "Technology::INTEL_18A";
            break;
        default:
            ostr << "Technology::unknown";
        }
        return ostr;
    }
    struct PerBitEnergy { double joulesPerBit; };
    struct PerBurstEnergy { double joulesPerBurst; };
    struct FixedEnergy { double joules; };

    using EnergyValue = std::variant<PerBitEnergy, PerBurstEnergy, FixedEnergy>;

    std::ostream& operator<<(std::ostream& os, const PerBitEnergy& energy) {
        os << "PerBit(" << energy.joulesPerBit << " J/bit)";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const PerBurstEnergy& energy) {
        os << "PerBurst(" << energy.joulesPerBurst << " J/burst)";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const FixedEnergy& energy) {
        os << "Fixed(" << energy.joules << " J)";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const EnergyValue& ev) {
        std::visit([&](const auto& value) {
            os << value;
            }, ev);
        return os;
    }

    using EventVariant = std::variant<uint32_t, std::string>;

    struct TechnologyEnergyData {
        Technology technology;
        std::map<EventType, std::map<EventVariant, EnergyValue>> energyMap;
    };

    std::ostream& operator<<(std::ostream& os, const TechnologyEnergyData& data) {
        os << "Technology: " << data.technology << "\n";
        os << "Energy Map:\n";
        for (const auto& eventPair : data.energyMap) {
            os << "  Event Type: " << eventPair.first << "\n";
            os << "    Parameters:\n";
            for (const auto& paramPair : eventPair.second) {
                os << "      ";
                std::visit([&](const auto& param) {
                    os << param;
                    }, paramPair.first);
                os << ": " << paramPair.second << "\n";
            }
        }
        return os;
    }

    class EnergyDatabase {
    private:
        std::map<Technology, TechnologyEnergyData> technologyEnergies;
        friend std::ostream& operator<<(std::ostream&, const EnergyDatabase&);

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

        std::optional<EnergyValue> getEnergyValue(Technology technology, EventType eventType, const EventVariant& parameter) const {
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
                    auto paramIt = eventIt->second.find(key.width);
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

    inline std::ostream& operator<<(std::ostream& ostr, const EnergyDatabase& db) {
        for (auto const& [key, value] : db.technologyEnergies) {
            ostr << key << " : " << value << '\n';
        }
        return ostr;
    }

}