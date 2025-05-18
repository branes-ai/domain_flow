#pragma once
#include <iostream>
#include <iomanip>

namespace sw::energy {

    struct EnergyReport {
        double instruction;
        double execute;
        double onchip_movement;
        double offchip_movement;
        double totalEnergy;
    };

    inline std::ostream& operator<<(std::ostream& os, const EnergyReport& energies) {
        constexpr unsigned WIDTH = 20;
        os << std::setw(WIDTH) << "Category";
        os << std::setw(WIDTH) << "Energy (J)";
        os << std::setw(WIDTH) << "Percentage (%)";
        os << '\n';

        auto precision = os.precision();
        os << " Compute\n";
        os << std::setw(WIDTH) << "Instruction" << std::setw(WIDTH) << energies.instruction << std::setw(WIDTH) << std::setprecision(2) << 100.0 * energies.instruction / energies.totalEnergy << std::setprecision(precision) << '\n';
        os << std::setw(WIDTH) << "Execute" << std::setw(WIDTH) << energies.execute << std::setw(WIDTH) << std::setprecision(2) << 100.0 * energies.execute / energies.totalEnergy << std::setprecision(precision) << '\n';
        os << " Data Movement\n";
        os << std::setw(WIDTH) << "On-chip" << std::setw(WIDTH) << energies.onchip_movement << std::setw(WIDTH) << std::setprecision(2) << 100.0 * energies.onchip_movement / energies.totalEnergy << std::setprecision(precision) << '\n';
        os << std::setw(WIDTH) << "Off-chip" << std::setw(WIDTH) << energies.offchip_movement << std::setw(WIDTH) << std::setprecision(2) << 100.0 * energies.offchip_movement / energies.totalEnergy << std::setprecision(precision) << '\n';
        os << " Total Energy\n";
        os << std::setw(WIDTH) << "Total" << std::setw(WIDTH) << energies.totalEnergy << std::setw(WIDTH) << "100" << '\n';
        os << std::setprecision(precision);
        return os;
    }

    inline EnergyReport calculateTotalEnergy(const EventCounterDatabase& eventDb, const EnergyDatabase& energyDb, Technology technology) {
        EnergyReport energies{0.0, 0.0, 0.0, 0.0};
        const auto* techData = energyDb.getTechnologyData(technology);

        if (!techData) {
            std::cerr << "Error: Energy data not found for technology." << std::endl;
            return energies;
        }

        // Process Compute Events
        for (const auto& [key, count] : eventDb.getComputeEventCounts()) {
            auto energyValueOpt = energyDb.getEnergyValue(technology, key);
            if (energyValueOpt.has_value()) {
                double eventEnergy = 0.0;
                if (std::holds_alternative<PerBitEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<PerBitEnergy>(energyValueOpt.value()).joulesPerBit * key.width * count;
                }
                else if (std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules * count;
                }
                if (key.isEventInstructionProcessing()) {
                    energies.instruction += eventEnergy;
                }
                else {
                    energies.execute += eventEnergy;
                }
                energies.totalEnergy += eventEnergy;
            }
            else {
                std::cerr << "Warning: Energy data not found for Compute Event: op=" << static_cast<int>(key.op) << ", size=" << static_cast<int>(key.width) << std::endl;
            }
        }

        // Process Memory Events
        for (const auto& [key, count] : eventDb.getMemoryEventCounts()) {
            auto energyValueOpt = energyDb.getEnergyValue(technology, key);
            if (energyValueOpt.has_value()) {
                double eventEnergy = 0.0;
                if (std::holds_alternative<PerBitEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<PerBitEnergy>(energyValueOpt.value()).joulesPerBit * key.width * count;
                }
                else if (std::holds_alternative<PerBurstEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<PerBurstEnergy>(energyValueOpt.value()).joulesPerBurst * key.burstLength * count;
                }
                else if (std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules * count;
                }
                if (key.isEventOffchip()) {
                    energies.offchip_movement += eventEnergy;
                }
                else {
                    energies.onchip_movement += eventEnergy;
                }
                energies.totalEnergy += eventEnergy;
            }
            else {
                std::cerr << "Warning: Energy data not found for Memory Event: op=" << static_cast<int>(key.op) << ", width=" << static_cast<int>(key.width) << ", burst=" << static_cast<int>(key.burstLength) << std::endl;
            }
        }

        // Process Network Events
        for (const auto& [key, count] : eventDb.getNetworkEventCounts()) {
            auto energyValueOpt = energyDb.getEnergyValue(technology, key);
            if (energyValueOpt.has_value()) {
                double eventEnergy = 0.0;
                if (std::holds_alternative<PerBitEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<PerBitEnergy>(energyValueOpt.value()).joulesPerBit * key.width * count;
                }
                else if (std::holds_alternative<PerBurstEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<PerBurstEnergy>(energyValueOpt.value()).joulesPerBurst * key.burstLength * count;
                }
                else if (std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                    eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules * count;
                }
                if (key.isEventOffchip()) {
                    energies.offchip_movement += eventEnergy;
                }
                else {
                    energies.onchip_movement += eventEnergy;
                }
                energies.totalEnergy += eventEnergy;
            }
            else {
                std::cerr << "Warning: Energy data not found for Network Event: op=" << static_cast<int>(key.op) << ", width=" << static_cast<int>(key.width) << ", burst=" << static_cast<int>(key.burstLength) << std::endl;
            }
        }

        return energies;
    }

}