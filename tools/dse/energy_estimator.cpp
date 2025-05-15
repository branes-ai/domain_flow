#include <iostream>
#include <numeric> // For std::accumulate
#include <optional>
#include <energy/energy.hpp>


int main() {
    using namespace sw::energy;

    // Create an EventCounterDatabase and record some event counts
    EventCounterDatabase eventCounterDb;
    eventCounterDb.increment(ComputeEvent{ EventType::ALU_ADD, 32 }, 1000);
    eventCounterDb.increment(ComputeEvent{ EventType::RF_READ, 32 }, 2000);
    eventCounterDb.increment(MemoryEvent{ EventType::DRAM_READ, 64, 8 }, 500);
    eventCounterDb.increment(NetworkEvent{ EventType::BUS_WRITE, 128, 1 }, 200);
    eventCounterDb.increment(ComputeEvent{ EventType::ALU_MUL, 8 }, 2000);
    eventCounterDb.increment(ComputeEvent{ EventType::VRF_READ, 256 }, 1000);

    // Create an EnergyDatabase and populate it with technology-specific data
    EnergyDatabase energyDb;

    // Energy data for TSMC 5nm
    TechnologyEnergyData tsmc5nmData{ Technology::TSMC_5NM };
    tsmc5nmData.energyMap[EventType::ALU_ADD][static_cast<uint32_t>(32)] = FixedEnergy{ 1.2e-15 };
    tsmc5nmData.energyMap[EventType::RF_READ][static_cast<uint32_t>(32)] = PerBitEnergy{ 3.75e-17 };
    tsmc5nmData.energyMap[EventType::DRAM_READ]["64_8"] = PerBurstEnergy{ 50e-12 }; // Using string key for width_burst
    tsmc5nmData.energyMap[EventType::BUS_WRITE]["128_1"] = PerBurstEnergy{ 10e-12 };
    tsmc5nmData.energyMap[EventType::ALU_MUL][static_cast<uint32_t>(8)] = FixedEnergy{ 0.8e-15 };
    tsmc5nmData.energyMap[EventType::VRF_READ][static_cast<uint32_t>(256)] = PerBitEnergy{ 3.75e-18 };
    energyDb.addTechnologyData(tsmc5nmData);
    std::cout << "TSMC : " << tsmc5nmData << '\n';

    // Energy data for Intel 18A
    TechnologyEnergyData intel18aData{ Technology::INTEL_18A };
    intel18aData.energyMap[EventType::ALU_ADD][static_cast<uint32_t>(32)] = FixedEnergy{ 1.0e-15 };
    intel18aData.energyMap[EventType::RF_READ][static_cast<uint32_t>(32)] = PerBitEnergy{ 3.5e-17 };
    intel18aData.energyMap[EventType::DRAM_READ]["64_8"] = PerBurstEnergy{ 45e-12 };
    intel18aData.energyMap[EventType::BUS_WRITE]["128_1"] = PerBurstEnergy{ 9e-12 };
    intel18aData.energyMap[EventType::ALU_MUL][static_cast<uint32_t>(8)] = FixedEnergy{ 0.7e-15 };
    intel18aData.energyMap[EventType::VRF_READ][static_cast<uint32_t>(256)] = PerBitEnergy{ 3.0e-18 };
    energyDb.addTechnologyData(intel18aData);
    std::cout << "Intel : " << intel18aData << '\n';

    std::cout << "energyDatabase  :\n" << energyDb << '\n';

    // Calculate and print energy consumption for different technologies
    double energyTSMC5nm = calculateTotalEnergy(eventCounterDb, energyDb, Technology::TSMC_5NM);
    std::cout << "Total energy (TSMC 5nm): " << energyTSMC5nm << " Joules" << std::endl;

    double energyIntel18A = calculateTotalEnergy(eventCounterDb, energyDb, Technology::INTEL_18A);
    std::cout << "Total energy (Intel 18A): " << energyIntel18A << " Joules" << std::endl;

    return EXIT_SUCCESS;
}