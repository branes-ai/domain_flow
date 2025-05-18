#include <iostream>
#include <numeric> // For std::accumulate
#include <optional>
#include <energy/energy.hpp>

namespace sw::energy {

    // HIGHEND CPU, TSMC N16t
    void addTechnologySample(TechnologyEnergyData& data) {

        // Front-end instruction processing
        data.energyMap[EventType::INSTR_FETCH][32u] = FixedEnergy{ 7.5e-12 }; // pJoules
        data.energyMap[EventType::INSTR_DECODE][32u] = FixedEnergy{ 3.5e-12 };
        data.energyMap[EventType::INSTR_DISPATCH][32u] = FixedEnergy{ 5.0e-12 };

        // Back-end pipeline processing
        data.energyMap[EventType::WRITE_BACK][32u] = FixedEnergy{ 5.0e-12 };

        // Execution stage Integer operations
        data.energyMap[EventType::ALU_IADD][8u] = FixedEnergy{ 0.35e-12 / 4.0 };
        data.energyMap[EventType::ALU_ISUB][8u] = FixedEnergy{ 0.35e-12 / 4.0 };
        data.energyMap[EventType::ALU_IMUL][8u] = FixedEnergy{ 2.25e-12 / 4.0 };
        data.energyMap[EventType::ALU_IDIV][8u] = FixedEnergy{ 5.0e-12 / 4.0 };
        data.energyMap[EventType::ALU_IMOD][8u] = FixedEnergy{ 5.0e-12 / 4.0 };
        data.energyMap[EventType::ALU_ISFU][8u] = FixedEnergy{ 5.0e-12 / 4.0 };

        data.energyMap[EventType::ALU_IADD][16u] = FixedEnergy{ 0.35e-12 / 2.0 };
        data.energyMap[EventType::ALU_ISUB][16u] = FixedEnergy{ 0.35e-12 / 2.0 };
        data.energyMap[EventType::ALU_IMUL][16u] = FixedEnergy{ 2.25e-12 / 2.0 };
        data.energyMap[EventType::ALU_IDIV][16u] = FixedEnergy{ 5.0e-12 / 2.0 };
        data.energyMap[EventType::ALU_IMOD][16u] = FixedEnergy{ 5.0e-12 / 2.0 };
        data.energyMap[EventType::ALU_ISFU][16u] = FixedEnergy{ 5.0e-12 / 2.0 };

        data.energyMap[EventType::ALU_IADD][32u] = FixedEnergy{ 0.35e-12 };
        data.energyMap[EventType::ALU_ISUB][32u] = FixedEnergy{ 0.35e-12 };
        data.energyMap[EventType::ALU_IMUL][32u] = FixedEnergy{ 2.25e-12 };
        data.energyMap[EventType::ALU_IDIV][32u] = FixedEnergy{ 5.0e-12 };
        data.energyMap[EventType::ALU_IMOD][32u] = FixedEnergy{ 5.0e-12 };
        data.energyMap[EventType::ALU_ISFU][32u] = FixedEnergy{ 10.0e-12 };

        data.energyMap[EventType::ALU_IADD][64u] = FixedEnergy{ 0.35e-12 * 2.0 };
        data.energyMap[EventType::ALU_ISUB][64u] = FixedEnergy{ 0.35e-12 * 2.0 };
        data.energyMap[EventType::ALU_IMUL][64u] = FixedEnergy{ 2.25e-12 * 2.0 };
        data.energyMap[EventType::ALU_IDIV][64u] = FixedEnergy{ 5.0e-12 * 2.0 };
        data.energyMap[EventType::ALU_IMOD][64u] = FixedEnergy{ 5.0e-12 * 2.0 };
        data.energyMap[EventType::ALU_ISFU][64u] = FixedEnergy{ 10.0e-12 * 2.0 };

        data.energyMap[EventType::ALU_IADD][128u] = FixedEnergy{ 0.35e-12 * 4.0 };
        data.energyMap[EventType::ALU_ISUB][128u] = FixedEnergy{ 0.35e-12 * 4.0 };
        data.energyMap[EventType::ALU_IMUL][128u] = FixedEnergy{ 2.25e-12 * 4.0 };
        data.energyMap[EventType::ALU_IDIV][128u] = FixedEnergy{ 5.0e-12 * 4.0 };
        data.energyMap[EventType::ALU_IMOD][128u] = FixedEnergy{ 5.0e-12 * 4.0 };
        data.energyMap[EventType::ALU_ISFU][128u] = FixedEnergy{ 10.0e-12 * 4.0 };

        // Execution stage Floating-Point operations
        data.energyMap[EventType::ALU_FADD][8u] = FixedEnergy{ 1.1e-12 / 4.0 };
        data.energyMap[EventType::ALU_FSUB][8u] = FixedEnergy{ 1.1e-12 / 4.0 };
        data.energyMap[EventType::ALU_FMUL][8u] = FixedEnergy{ 3.7e-12 / 4.0 };
        data.energyMap[EventType::ALU_FDIV][8u] = FixedEnergy{ 6.5e-12 / 4.0 };
        data.energyMap[EventType::ALU_FMA][8u]  = FixedEnergy{ 4.0e-12 / 4.0 };
        data.energyMap[EventType::ALU_FSFU][8u] = FixedEnergy{ 10.0e-12 / 4.0 };

        data.energyMap[EventType::ALU_FADD][16u] = FixedEnergy{ 1.1e-12 / 2.0 };
        data.energyMap[EventType::ALU_FSUB][16u] = FixedEnergy{ 1.1e-12 / 2.0 };
        data.energyMap[EventType::ALU_FMUL][16u] = FixedEnergy{ 3.7e-12 / 2.0 };
        data.energyMap[EventType::ALU_FDIV][16u] = FixedEnergy{ 6.5e-12 / 2.0 };
        data.energyMap[EventType::ALU_FMA][16u]  = FixedEnergy{ 4.0e-12 / 2.0 };
        data.energyMap[EventType::ALU_FSFU][16u] = FixedEnergy{ 10.0e-12 / 2.0 };

        data.energyMap[EventType::ALU_FADD][32u] = FixedEnergy{ 1.1e-12 };
        data.energyMap[EventType::ALU_FSUB][32u] = FixedEnergy{ 1.1e-12 };
        data.energyMap[EventType::ALU_FMUL][32u] = FixedEnergy{ 3.7e-12 };
        data.energyMap[EventType::ALU_FDIV][32u] = FixedEnergy{ 6.5e-12 };
        data.energyMap[EventType::ALU_FMA][32u]  = FixedEnergy{ 4.0e-12 };
        data.energyMap[EventType::ALU_FSFU][32u] = FixedEnergy{ 10.0e-12 };

        data.energyMap[EventType::ALU_FADD][64u] = FixedEnergy{ 1.1e-12 * 2.0 };
        data.energyMap[EventType::ALU_FSUB][64u] = FixedEnergy{ 1.1e-12 * 2.0 };
        data.energyMap[EventType::ALU_FMUL][64u] = FixedEnergy{ 3.7e-12 * 2.0 };
        data.energyMap[EventType::ALU_FDIV][64u] = FixedEnergy{ 6.5e-12 * 2.0 };
        data.energyMap[EventType::ALU_FMA][64u]  = FixedEnergy{ 4.0e-12 * 2.0 };
        data.energyMap[EventType::ALU_FSFU][64u] = FixedEnergy{ 10.0e-12 * 2.0 };

        data.energyMap[EventType::ALU_FADD][128u] = FixedEnergy{ 1.1e-12 * 4.0 };
        data.energyMap[EventType::ALU_FSUB][128u] = FixedEnergy{ 1.1e-12 * 4.0 };
        data.energyMap[EventType::ALU_FMUL][128u] = FixedEnergy{ 3.7e-12 * 4.0 };
        data.energyMap[EventType::ALU_FDIV][128u] = FixedEnergy{ 6.5e-12 * 4.0 };
        data.energyMap[EventType::ALU_FMA][128u]  = FixedEnergy{ 4.0e-12 * 4.0 };
        data.energyMap[EventType::ALU_FSFU][128u] = FixedEnergy{ 10.0e-12 * 4.0 };

        // Register file operations
        data.energyMap[EventType::RF_READ][32u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][32u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::RF_READ][64u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][64u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::RF_READ][128u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][128u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::RF_READ][256u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][256u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::RF_READ][512u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][512u] = PerBitEnergy{ 0.6e-12 };

        data.energyMap[EventType::VRF_READ][32u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::VRF_WRITE][32u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::VRF_READ][64u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::VRF_WRITE][64u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::VRF_READ][128u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::VRF_WRITE][128u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::VRF_READ][256u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::VRF_WRITE][256u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::VRF_READ][512u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::VRF_WRITE][512u] = PerBitEnergy{ 0.6e-12 };

        // Cache access operations
        data.energyMap[EventType::L1_CACHE_READ][128u] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::L1_CACHE_WRITE][128u] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::L1_CACHE_READ][256u] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::L1_CACHE_WRITE][256u] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::L1_CACHE_READ][512u] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::L1_CACHE_WRITE][512u] = PerBitEnergy{ 3.0e-12 };

        data.energyMap[EventType::L2_CACHE_READ][128u] = PerBitEnergy{ 4.0e-12 };
        data.energyMap[EventType::L2_CACHE_WRITE][128u] = PerBitEnergy{ 6.0e-12 };
        data.energyMap[EventType::L2_CACHE_READ][256u] = PerBitEnergy{ 4.0e-12 };
        data.energyMap[EventType::L2_CACHE_WRITE][256u] = PerBitEnergy{ 6.0e-12 };
        data.energyMap[EventType::L2_CACHE_READ][512u] = PerBitEnergy{ 4.0e-12 };
        data.energyMap[EventType::L2_CACHE_WRITE][512u] = PerBitEnergy{ 6.0e-12 };

        data.energyMap[EventType::L3_CACHE_READ][128u] = PerBitEnergy{ 6.0e-12 };
        data.energyMap[EventType::L3_CACHE_WRITE][128u] = PerBitEnergy{ 12.0e-12 };
        data.energyMap[EventType::L3_CACHE_READ][256u] = PerBitEnergy{ 6.0e-12 };
        data.energyMap[EventType::L3_CACHE_WRITE][256u] = PerBitEnergy{ 12.0e-12 };
        data.energyMap[EventType::L3_CACHE_READ][512u] = PerBitEnergy{ 6.0e-12 };
        data.energyMap[EventType::L3_CACHE_WRITE][512u] = PerBitEnergy{ 12.0e-12 };

        // DRAM memory access operations
        data.energyMap[EventType::DRAM_READ][128u] = PerBitEnergy{ 15.0e-12 };
        data.energyMap[EventType::DRAM_WRITE][128u] = PerBitEnergy{ 20.0e-12 };
        data.energyMap[EventType::DRAM_READ][256u] = PerBitEnergy{ 15.0e-12 };
        data.energyMap[EventType::DRAM_WRITE][256u] = PerBitEnergy{ 20.0e-12 };
        data.energyMap[EventType::DRAM_READ][512u] = PerBitEnergy{ 15.0e-12 };
        data.energyMap[EventType::DRAM_WRITE][512u] = PerBitEnergy{ 20.0e-12 };

        // Network access operations
        data.energyMap[EventType::BUS_READ]["64_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::BUS_WRITE]["64_1"] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::BUS_READ]["64_2"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::BUS_WRITE]["64_2"] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::BUS_READ]["128_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::BUS_WRITE]["128_1"] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::BUS_READ]["256_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::BUS_WRITE]["256_1"] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::BUS_READ]["512_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::BUS_WRITE]["512_1"] = PerBitEnergy{ 3.0e-12 };

        data.energyMap[EventType::NETWORK_READ]["64_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::NETWORK_WRITE]["64_1"] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::NETWORK_READ]["128_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::NETWORK_WRITE]["128_1"] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::NETWORK_READ]["256_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::NETWORK_WRITE]["256_1"] = PerBitEnergy{ 3.0e-12 };
        data.energyMap[EventType::NETWORK_READ]["512_1"] = PerBitEnergy{ 2.0e-12 };
        data.energyMap[EventType::NETWORK_WRITE]["512_1"] = PerBitEnergy{ 3.0e-12 };
    }
}

int main() {
    using namespace sw::energy;

    // Create an EventCounterDatabase and record some event counts
    EventCounterDatabase eventCounterDb;
    eventCounterDb.increment(ComputeEvent{ EventType::ALU_IADD, 32 }, 1000);
    eventCounterDb.increment(MemoryEvent{ EventType::RF_READ, 32 }, 2000);
    eventCounterDb.increment(MemoryEvent{ EventType::DRAM_READ, 64, 8 }, 500);
    eventCounterDb.increment(NetworkEvent{ EventType::BUS_WRITE, 128, 1 }, 200);
    eventCounterDb.increment(ComputeEvent{ EventType::ALU_FMUL, 8 }, 2000);
    eventCounterDb.increment(MemoryEvent{ EventType::VRF_READ, 256 }, 1000);

    // Create an EnergyDatabase and populate it with technology-specific data
    EnergyDatabase energyDb;

    // Energy data for TSMC 5nm
    TechnologyEnergyData tsmc5nmData{ Technology::TSMC_5NM, DesignType::LOWPOWER_STANDARD_CELL };
    tsmc5nmData.energyMap[EventType::ALU_IADD][static_cast<uint32_t>(32)] = FixedEnergy{ 1.2e-15 };
    tsmc5nmData.energyMap[EventType::RF_READ][static_cast<uint32_t>(32)] = PerBitEnergy{ 3.75e-17 };
    tsmc5nmData.energyMap[EventType::DRAM_READ]["64_8"] = PerBurstEnergy{ 50e-12 }; // Using string key for width_burst
    tsmc5nmData.energyMap[EventType::BUS_WRITE]["128_1"] = PerBurstEnergy{ 10e-12 };
    tsmc5nmData.energyMap[EventType::ALU_FMUL][static_cast<uint32_t>(8)] = FixedEnergy{ 0.8e-15 };
    tsmc5nmData.energyMap[EventType::VRF_READ][static_cast<uint32_t>(256)] = PerBitEnergy{ 3.75e-18 };
    energyDb.addTechnologyData(tsmc5nmData);
    std::cout << "TSMC : " << tsmc5nmData << '\n';

    // Energy data for Intel 18A
    TechnologyEnergyData intel18aData{ Technology::INTEL_18A, DesignType::DESKTOP_CPU };
    intel18aData.energyMap[EventType::ALU_IADD][static_cast<uint32_t>(32)] = FixedEnergy{ 1.0e-15 };
    intel18aData.energyMap[EventType::RF_READ][static_cast<uint32_t>(32)] = PerBitEnergy{ 3.5e-17 };
    intel18aData.energyMap[EventType::DRAM_READ]["64_8"] = PerBurstEnergy{ 45e-12 };
    intel18aData.energyMap[EventType::BUS_WRITE]["128_1"] = PerBurstEnergy{ 9e-12 };
    intel18aData.energyMap[EventType::ALU_FMUL][static_cast<uint32_t>(8)] = FixedEnergy{ 0.7e-15 };
    intel18aData.energyMap[EventType::VRF_READ][static_cast<uint32_t>(256)] = PerBitEnergy{ 3.0e-18 };
    energyDb.addTechnologyData(intel18aData);
    std::cout << "Intel : " << intel18aData << '\n';

    std::cout << "energyDatabase  :\n" << energyDb << '\n';

    // Calculate and print energy consumption for different technologies
    double energyTSMC5nm = calculateTotalEnergy(eventCounterDb, energyDb, Technology::TSMC_5NM);
    std::cout << "Total energy (TSMC 5nm): " << energyTSMC5nm << " Joules" << std::endl;

    double energyIntel18A = calculateTotalEnergy(eventCounterDb, energyDb, Technology::INTEL_18A);
    std::cout << "Total energy (Intel 18A): " << energyIntel18A << " Joules" << std::endl;


    TechnologyEnergyData tech(Technology::TSMC_16NM, DesignType::MOBILE_CPU );
    addTechnologySample(tech);
    EnergyDatabase db;
    db.addTechnologyData(tech);
    db.serializeToCSV("testDb.csv");
    std::cout << "*****************************************\n" << db << '\n';
    EnergyDatabase dbIn;
    dbIn.deserializeFromCSV("testDb.csv");
    std::cout << "energyDatabase:\n" << dbIn << '\n';

    return EXIT_SUCCESS;
}