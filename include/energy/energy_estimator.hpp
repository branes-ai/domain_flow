#pragma once

namespace sw::energy {

    inline double calculateTotalEnergy(const EventCounterDatabase& eventDb, const EnergyDatabase& energyDb, Technology technology) {
        double totalEnergy = 0.0;
        const auto* techData = energyDb.getTechnologyData(technology);

        if (!techData) {
            std::cerr << "Error: Energy data not found for technology." << std::endl;
            return 0.0;
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
                totalEnergy += eventEnergy;
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
                totalEnergy += eventEnergy;
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
                totalEnergy += eventEnergy;
            }
            else {
                std::cerr << "Warning: Energy data not found for Network Event: op=" << static_cast<int>(key.op) << ", width=" << static_cast<int>(key.width) << ", burst=" << static_cast<int>(key.burstLength) << std::endl;
            }
        }

        return totalEnergy;
    }

}