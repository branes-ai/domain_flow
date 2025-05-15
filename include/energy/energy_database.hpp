#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
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

    inline std::istream& operator>>(std::istream& istr, Technology& tech) {
        std::string techStr;
        istr >> techStr;
        if (techStr == "TSMC_5NM") {
            tech = Technology::TSMC_5NM;
        }
        else if (techStr == "INTEL_18A") {
            tech = Technology::INTEL_18A;
        }
        else {
            tech = static_cast<Technology>(-1); // Or throw an exception for unknown technology
        }
        return istr;
    }

    struct PerBitEnergy { double joulesPerBit; };
    struct PerBurstEnergy { double joulesPerBurst; };
    struct FixedEnergy { double joules; };

    std::ostream& operator<<(std::ostream& os, const PerBitEnergy& energy) {
        os << "PerBit(" << energy.joulesPerBit << " J/bit)";
        return os;
    }
    std::istream& operator>>(std::istream& is, PerBitEnergy& energy) {
        std::string type;
        char comma;
        if (is >> type >> comma && type == "PerBit" && comma == ',') {
            is >> energy.joulesPerBit;
        }
        else {
            is.setstate(std::ios::failbit);
        }
        return is;
    }

    std::ostream& operator<<(std::ostream& os, const PerBurstEnergy& energy) {
        os << "PerBurst(" << energy.joulesPerBurst << " J/burst)";
        return os;
    }
    std::istream& operator>>(std::istream& is, PerBurstEnergy& energy) {
        std::string type;
        char comma;
        if (is >> type >> comma && type == "PerBurst" && comma == ',') {
            is >> energy.joulesPerBurst;
        }
        else {
            is.setstate(std::ios::failbit);
        }
        return is;
    }

    std::ostream& operator<<(std::ostream& os, const FixedEnergy& energy) {
        os << "Fixed(" << energy.joules << " J)";
        return os;
    }
    std::istream& operator>>(std::istream& is, FixedEnergy& energy) {
        std::string type;
        char comma;
        if (is >> type >> comma && type == "Fixed" && comma == ',') {
            is >> energy.joules;
        }
        else {
            is.setstate(std::ios::failbit);
        }
        return is;
    }

    using EnergyValue = std::variant<PerBitEnergy, PerBurstEnergy, FixedEnergy>;

    std::ostream& operator<<(std::ostream& os, const EnergyValue& ev) {
        std::visit([&](const auto& value) {
            os << value;
            }, ev);
        return os;
    }
    std::istream& operator>>(std::istream& is, EnergyValue& ev) {
        std::string type;
        std::istream::pos_type currentPos = is.tellg();
        is >> type;
        is.seekg(currentPos); // Reset the stream position

        if (type == "PerBit") {
            ev.emplace<PerBitEnergy>();
            is >> std::get<PerBitEnergy>(ev);
        }
        else if (type == "PerBurst") {
            ev.emplace<PerBurstEnergy>();
            is >> std::get<PerBurstEnergy>(ev);
        }
        else if (type == "Fixed") {
            ev.emplace<FixedEnergy>();
            is >> std::get<FixedEnergy>(ev);
        }
        else {
            is.setstate(std::ios::failbit);
        }
        return is;
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

    std::istream& operator>>(std::istream& is, TechnologyEnergyData& data) {
        std::string line;
        if (std::getline(is, line)) {
            std::stringstream ss(line);
            std::string token;
            char delimiter = ',';

            if (std::getline(ss, token, delimiter)) {
                std::stringstream techStream(token);
                techStream >> data.technology;
                if (techStream.fail()) return is; // Failed to read technology
            }

            while (std::getline(ss, token, delimiter)) {
                EventType eventType;
                std::stringstream eventStream(token);
                eventStream >> eventType;
                if (eventStream.fail()) return is;

                std::map<EventVariant, EnergyValue> eventEnergyMap;
                while (std::getline(ss, token, delimiter)) {
                    EventVariant eventParam;
                    if (token.find_first_not_of("0123456789") == std::string::npos) {
                        eventParam = static_cast<uint32_t>(std::stoul(token));
                    }
                    else {
                        eventParam = token;
                    }

                    std::getline(ss, token, delimiter);
                    EnergyValue energyValue;
                    std::stringstream energyStream(token);
                    energyStream >> energyValue;
                    if (energyStream.fail()) return is;

                    eventEnergyMap[eventParam] = energyValue;
                }
                data.energyMap[eventType] = eventEnergyMap;
                break; // Assuming one event type per line for simplicity in reading
            }
        }
        return is;
    }

    class EnergyDatabase {
    private:
        std::map<Technology, TechnologyEnergyData> technologyEnergies;
        friend std::ostream& operator<<(std::ostream&, const EnergyDatabase&);
        friend std::istream& operator>>(std::istream&, EnergyDatabase&);

    public:
        void clear() {
            technologyEnergies.clear();
        }

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

        bool serializeToCSV(const std::string& filename) const {
            std::ofstream outfile(filename);
            if (!outfile.is_open()) {
                std::cerr << "Error opening file: " << filename << std::endl;
                return false;
            }

            // Write header (Technology, EventType, Parameter, EnergyType, EnergyValue)
            outfile << "Technology,EventType,Parameter,EnergyType,EnergyValue\n";

            for (const auto& [tech, techData] : technologyEnergies) {
                for (const auto& [eventType, eventMap] : techData.energyMap) {
                    for (const auto& [param, energyValue] : eventMap) {
                        outfile << tech << "," << eventType << ",";
                        std::visit([&](const auto& p) {
                            outfile << p;
                            }, param);
                        outfile << ",";
                        std::visit([&](const auto& ev) {
                            if constexpr (std::is_same_v<std::decay_t<decltype(ev)>, PerBitEnergy>) {
                                outfile << "PerBit," << ev.joulesPerBit;
                            }
                            else if constexpr (std::is_same_v<std::decay_t<decltype(ev)>, PerBurstEnergy>) {
                                outfile << "PerBurst," << ev.joulesPerBurst;
                            }
                            else if constexpr (std::is_same_v<std::decay_t<decltype(ev)>, FixedEnergy>) {
                                outfile << "Fixed," << ev.joules;
                            }
                            }, energyValue);
                        outfile << "\n";
                    }
                }
            }

            outfile.close();
            return true;
        }

        bool deserializeFromCSV(const std::string& filename) {
            std::ifstream infile(filename);
            if (!infile.is_open()) {
                std::cerr << "Error opening file: " << filename << std::endl;
                return false;
            }

            std::string line;
            std::getline(infile, line); // Read and discard the header line

            technologyEnergies.clear();
            while (std::getline(infile, line)) {
                std::stringstream ss(line);
                std::string token;
                std::vector<std::string> tokens;
                while (std::getline(ss, token, ',')) {
                    tokens.push_back(token);
                }

                if (tokens.size() >= 5) {
                    Technology tech;
                    std::stringstream techStream(tokens[0]);
                    techStream >> tech;
                    if (techStream.fail()) continue;

                    EventType eventType;
                    std::stringstream eventStream(tokens[1]);
                    eventStream >> eventType;
                    if (eventStream.fail()) continue;

                    EventVariant parameter;
                    if (tokens[2].find_first_not_of("0123456789") == std::string::npos) {
                        parameter = static_cast<uint32_t>(std::stoul(tokens[2]));
                    }
                    else {
                        parameter = tokens[2];
                    }

                    EnergyValue energyValue;
                    if (tokens[3] == "PerBit" && tokens.size() == 5) {
                        try {
                            energyValue.emplace<PerBitEnergy>(std::stod(tokens[4]));
                        }
                        catch (const std::invalid_argument& e) { /* Handle error */ continue; }
                    }
                    else if (tokens[3] == "PerBurst" && tokens.size() == 5) {
                        try {
                            energyValue.emplace<PerBurstEnergy>(std::stod(tokens[4]));
                        }
                        catch (const std::invalid_argument& e) { /* Handle error */ continue; }
                    }
                    else if (tokens[3] == "Fixed" && tokens.size() == 5) {
                        try {
                            energyValue.emplace<FixedEnergy>(std::stod(tokens[4]));
                        }
                        catch (const std::invalid_argument& e) { /* Handle error */ continue; }
                    }
                    else {
                        continue; // Invalid energy type or missing value
                    }

                    technologyEnergies[tech].energyMap[eventType][parameter] = energyValue;
                }
            }

            infile.close();
            return true;
        }
    };

    inline std::ostream& operator<<(std::ostream& ostr, const EnergyDatabase& db) {
        for (auto const& [key, value] : db.technologyEnergies) {
            ostr << key << " : " << value << '\n';
        }
        return ostr;
    }

}