#include <iostream>
#include <numeric> // For std::accumulate
#include <optional>
#include <energy/energy_database.hpp>
#include <energy/event_database.hpp>

namespace sw::energy {
    double calculateTotalEnergy(const EventDatabase& eventDb, const EnergyDatabase& energyDb, Technology technology) {
        double totalEnergy = 0.0;
        const auto& events = eventDb.getEvents();
        const auto* techData = energyDb.getTechnologyData(technology);

        if (!techData) {
            std::cerr << "Error: Energy data not found for technology." << std::endl;
            return 0.0;
        }

        for (const auto& event : events) {
            double eventEnergy = 0.0;
            std::visit([&](const auto& data) {
                std::optional<EnergyValue> energyValueOpt;
                if constexpr (std::is_same_v<std::decay_t<decltype(data)>, ALUIntegerAddEvent>) {
                    energyValueOpt = energyDb.getEnergyValue(technology, event.type, data.size);
                    if (energyValueOpt.has_value()) {
                        if (std::holds_alternative<PerBitEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<PerBitEnergy>(energyValueOpt.value()).joulesPerBit * static_cast<int>(data.size);
                        }
                        else if (std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules;
                        }
                    }
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(data)>, RegisterReadEvent>) {
                    energyValueOpt = energyDb.getEnergyValue(technology, event.type, data.size);
                    if (energyValueOpt.has_value()) {
                        if (std::holds_alternative<PerBitEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<PerBitEnergy>(energyValueOpt.value()).joulesPerBit * static_cast<int>(data.size);
                        }
                        else if (std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules;
                        }
                    }
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(data)>, CacheAccessEvent>) {
                    energyValueOpt = energyDb.getEnergyValue(technology, event.type, data.size);
                    if (energyValueOpt.has_value()) {
                        if (std::holds_alternative<PerBitEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<PerBitEnergy>(energyValueOpt.value()).joulesPerBit * static_cast<int>(data.size);
                        }
                        else if (std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules;
                        }
                    }
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(data)>, DRAMAccessEvent>) {
                    energyValueOpt = energyDb.getEnergyValue(technology, event.type, "burst"); // Assuming "burst" is the key
                    if (energyValueOpt.has_value() && std::holds_alternative<PerBurstEnergy>(energyValueOpt.value())) {
                        eventEnergy = std::get<PerBurstEnergy>(energyValueOpt.value()).joulesPerBurst * data.burstCount;
                    }
                    else if (energyValueOpt.has_value() && std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                        eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules;
                    }
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(data)>, BusAccessEvent>) {
                    energyValueOpt = energyDb.getEnergyValue(technology, event.type, data.busType); // Keyed by bus type
                    if (energyValueOpt.has_value()) {
                        if (std::holds_alternative<PerBitEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<PerBitEnergy>(energyValueOpt.value()).joulesPerBit * data.burstLength * 64.0; // Assuming 64 bits per burst
                        }
                        else if (std::holds_alternative<PerBurstEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<PerBurstEnergy>(energyValueOpt.value()).joulesPerBurst * data.burstLength;
                        }
                        else if (std::holds_alternative<FixedEnergy>(energyValueOpt.value())) {
                            eventEnergy = std::get<FixedEnergy>(energyValueOpt.value()).joules;
                        }
                    }
                }

                if (!energyValueOpt.has_value()) {
                    std::cerr << "Warning: Energy data not found for event type: " << static_cast<int>(event.type) << " and parameters." << std::endl;
                }
                }, event.data);
            totalEnergy += eventEnergy;
        }

        return totalEnergy;
    }

}
int main() {
    using namespace sw::energy;

    // Create an EventDatabase and add some events
    EventDatabase eventDb;
    eventDb.addEvent({EventType::ALU_INTEGER_ADD, ALUIntegerAddEvent{IntegerSizeType::BITS_32}});
    eventDb.addEvent({EventType::REGISTER_READ, RegisterReadEvent{RegisterSizeType::BITS_64, 0x1000}});
    eventDb.addEvent({EventType::DRAM_ACCESS, DRAMAccessEvent{DRAMAccessEvent::AccessType::READ, 4}});
    eventDb.addEvent({EventType::BUS_ACCESS, BusAccessEvent{BusAccessEvent::AccessType::READ, "AXI", 10}});

    // Create an EnergyDatabase and populate it with technology-specific data
    EnergyDatabase energyDb;

    // Energy data for TSMC 5nm
    TechnologyEnergyData tsmc5nmData{Technology::TSMC_5NM};
    tsmc5nmData.energyMap[EventType::ALU_INTEGER_ADD][IntegerSizeType::BITS_32] = PerBitEnergy{1.2e-15};
    tsmc5nmData.energyMap[EventType::REGISTER_READ][RegisterSizeType::BITS_64] = PerBitEnergy{0.8e-15};
    tsmc5nmData.energyMap[EventType::DRAM_ACCESS]["burst"] = PerBurstEnergy{50e-12};
    tsmc5nmData.energyMap[EventType::BUS_ACCESS]["AXI"] = PerBurstEnergy{10e-12};
    energyDb.addTechnologyData(tsmc5nmData);

    // Energy data for Intel 18A
    TechnologyEnergyData intel18aData{Technology::INTEL_18A};
    intel18aData.energyMap[EventType::ALU_INTEGER_ADD][IntegerSizeType::BITS_32] = PerBitEnergy{1.0e-15};
    intel18aData.energyMap[EventType::REGISTER_READ][RegisterSizeType::BITS_64] = PerBitEnergy{0.7e-15};
    intel18aData.energyMap[EventType::DRAM_ACCESS]["burst"] = PerBurstEnergy{45e-12};
    intel18aData.energyMap[EventType::BUS_ACCESS]["AXI"] = PerBurstEnergy{9e-12};
    energyDb.addTechnologyData(intel18aData);

    // Calculate and print energy consumption for different technologies
    double energyTSMC5nm = calculateTotalEnergy(eventDb, energyDb, Technology::TSMC_5NM);
    std::cout << "Total energy (TSMC 5nm): " << energyTSMC5nm << " Joules" << std::endl;

    double energyIntel18A = calculateTotalEnergy(eventDb, energyDb, Technology::INTEL_18A);
    std::cout << "Total energy (Intel 18A): " << energyIntel18A << " Joules" << std::endl;

    return EXIT_SUCCESS;
}