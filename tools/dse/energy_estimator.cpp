#include <iostream>
#include <numeric> // For std::accumulate
#include <optional>
#include <energy/energy.hpp>

namespace sw::energy {

    // HIGHEND CPU, TSMC N16t
    void addTechnologySample(TechnologyEnergyData& data) {

        data.technology = Technology::TSMC_16NM;
        data.design = DesignType::HIGHEND_CPU;

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
        data.energyMap[EventType::RF_READ][4u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][4u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::RF_READ][8u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][8u] = PerBitEnergy{ 0.6e-12 };
        data.energyMap[EventType::RF_READ][16u] = PerBitEnergy{ 0.4e-12 };
        data.energyMap[EventType::RF_WRITE][16u] = PerBitEnergy{ 0.6e-12 };
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
        // per-bit Read energy is 15.0e-12
        // per-bit Write energy is 20.0e-12
        // we aggregated that into a perBurstEnergy and then will multiply that by the burst length
        data.energyMap[EventType::DRAM_READ]["64_2"] = PerBurstEnergy{ 64*15.0e-12};
        data.energyMap[EventType::DRAM_WRITE]["64_2"] = PerBurstEnergy{ 64*20.0e-12};
        data.energyMap[EventType::DRAM_READ]["64_4"] = PerBurstEnergy{ 64*15.0e-12};
        data.energyMap[EventType::DRAM_WRITE]["64_4"] = PerBurstEnergy{ 64*20.0e-12 };
        data.energyMap[EventType::DRAM_READ]["64_8"] = PerBurstEnergy{ 64*15.0e-12 };
        data.energyMap[EventType::DRAM_WRITE]["64_8"] = PerBurstEnergy{ 64*20.0e-12 };

        // Network access operations
        // TODO: Also need to be transformed into PerBurstEnergy
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


    void testSerialization() {
        TechnologyEnergyData tech(Technology::TSMC_16NM, DesignType::MOBILE_CPU);
        addTechnologySample(tech);
        EnergyDatabase db;
        db.addTechnologyData(tech);
        db.serializeToCSV("testDb.csv");
        std::cout << "*****************************************\n" << db << '\n';
        EnergyDatabase dbIn;
        dbIn.deserializeFromCSV("testDb.csv");
        std::cout << "energyDatabase:\n" << dbIn << '\n';
    }


    void cpu_matmul(unsigned m, unsigned n, unsigned k, unsigned widthInBits, EventCounterDatabase& db) {
        // we are going to have the A and the B matrix coming from DRAM
        // A(m,k) * B(k, n) = C(m,n)
        const unsigned DRAM_TRANSACTION_SIZE = 64; // bytes
        unsigned elementWidthInBytes = widthInBits / 8;
        unsigned nrOfDramReadsA = ((m * k * elementWidthInBytes) / DRAM_TRANSACTION_SIZE) + 1;
        unsigned nrOfDramReadsB = ((k * n * elementWidthInBytes) / DRAM_TRANSACTION_SIZE) + 1;
        unsigned nrOfDramReads = nrOfDramReadsA + nrOfDramReadsB;
        db.increment(MemoryEvent{ EventType::DRAM_READ, 64, 8 }, nrOfDramReads);

        // calculate the actual computations
        unsigned nrFMAs = m * k * n;
        db.increment(ComputeEvent{ EventType::ALU_FMA, widthInBits }, nrFMAs);
        // and the RF events
        db.increment(MemoryEvent{ EventType::RF_READ, widthInBits, 0 }, 3*nrFMAs);
        db.increment(MemoryEvent{ EventType::RF_WRITE, widthInBits, 0 }, nrFMAs);

        // we need to execute loop indices and address generation for each FMA
        // assume 10 instructions per iteration
        db.increment(ComputeEvent{ EventType::INSTR_FETCH, 32 }, nrFMAs);
        db.increment(ComputeEvent{ EventType::INSTR_DECODE, 32 }, nrFMAs);
        db.increment(ComputeEvent{ EventType::INSTR_DISPATCH, 32 }, nrFMAs);

        // and we need to write the result matrix back to memory
        unsigned nrOfDramWritesC = ((m * n * elementWidthInBytes) / DRAM_TRANSACTION_SIZE) + 1;
        db.increment(MemoryEvent{ EventType::DRAM_WRITE, 64, 8 }, nrOfDramWritesC);

        // what is the latency of the number of DRAM access that you need to make?
        // assume we are bw limited
        // DDR4
        // 2133 MT/s = 2133 * 64 / 8 * 1000 = 17.06 GB/s
        // 2400 MT/s = 19.2 GB/s
        // 3200 MT/s = 25.6 GB/s
        //
        // Dual Channel configurations
        // 2133 MT/s = 34.12 GB/s
        // 2400 MT/s = 38.4 GB/s
        // 3200 MT/s = 51.2 GB/s
        std::cout << "Latency of DRAM accesses\n";
        std::cout << "2133 MT/s DDR4 @ 34.12 GB/s : " << nrOfDramReads * 64.0 / 34.12e9 << "sec\n";
        std::cout << "2400 MT/s DDR4 @ 38.4 GB/s : " << nrOfDramReads * 64.0 / 38.4e9 << "sec\n";
        std::cout << "3200 MT/s DDR4 @ 51.2 GB/s : " << nrOfDramReads * 64.0 / 51.2e9 << "sec\n";
    }
}


int main() {
    using namespace sw::energy;
    constexpr bool bTrace = false;

    // Energy data for TSMC 16nm, High-end CPU design
    TechnologyEnergyData tsmc16nmData;
    addTechnologySample(tsmc16nmData);
    // Create an EnergyDatabase and populate it with technology-specific data
    EnergyDatabase energyDb;
    energyDb.addTechnologyData(tsmc16nmData);

    // x and y samples
    std::vector<double> x, y;
   
    // dimensions of the problem
    unsigned m{ 256 }, n{ 256 }, k{ 256 };
    double clockPeriod = 1.0e-9;

    double latency = clockPeriod * (m + n + k);
    EnergyReport energies;

    // Create an EventCounterDatabase and record some event counts
    EventCounterDatabase fp32_matmul;
    cpu_matmul(256, 256, 256, 32, fp32_matmul);
    // Calculate and print energy consumption for different technologies
    energies = calculateTotalEnergy(fp32_matmul, energyDb, Technology::TSMC_16NM);
    if constexpr (bTrace) std::cout << "Energy Report 256x256 matmul fp32\n" << energies << std::endl;
    //Latency of DRAM accesses
    //    2133 MT / s DDR4 @ 34.12 GB / s : 1.53698e-05sec
    //    2400 MT / s DDR4 @ 38.4 GB / s : 1.36567e-05sec             <----- as representative memory technology
    //    3200 MT / s DDR4 @ 51.2 GB / s : 1.02425e-05sec
    x.push_back(1.36567e-05);
    y.push_back(energies.totalEnergy);
    // speed of light
    x.push_back(latency);
    y.push_back(energies.totalEnergy);

    EventCounterDatabase fp16_matmul;
    cpu_matmul(256, 256, 256, 16, fp16_matmul);
    energies = calculateTotalEnergy(fp16_matmul, energyDb, Technology::TSMC_16NM);
    if constexpr (bTrace) std::cout << "Energy Report 256x256 matmul fp16\n" << energies << std::endl;
    //Latency of DRAM accesses
    //    2133 MT / s DDR4 @ 34.12 GB / s : 7.68675e-06sec
    //    2400 MT / s DDR4 @ 38.4 GB / s : 6.83e-06sec             <----- as representative memory technology
    //    3200 MT / s DDR4 @ 51.2 GB / s : 5.1225e-06sec
    x.push_back(6.83e-06);
    y.push_back(energies.totalEnergy);
    // speed of light
    x.push_back(latency);
    y.push_back(energies.totalEnergy);

    EventCounterDatabase fp8_matmul;
    cpu_matmul(256, 256, 256, 8, fp8_matmul);
    energies = calculateTotalEnergy(fp8_matmul, energyDb, Technology::TSMC_16NM);
    if constexpr (bTrace) std::cout << "Energy Report 256x256 matmul fp8\n" << energies << std::endl;
    //Latency of DRAM accesses
    //    2133 MT / s DDR4 @ 34.12 GB / s : 3.84525e-06sec
    //    2400 MT / s DDR4 @ 38.4 GB / s : 3.41667e-06sec            <----- as representative memory technology
    //    3200 MT / s DDR4 @ 51.2 GB / s : 2.5625e-06sec
    x.push_back(3.41667e-06);
    y.push_back(energies.totalEnergy);
    // speed of light
    x.push_back(latency);
    y.push_back(energies.totalEnergy);

    constexpr unsigned WIDTH = 15;
    std::cout << "  latency (sec) :     energy (J)  :      power (W)\n";
    for (size_t i = 0; i < x.size(); ++i) {
        std::cout << std::setw(WIDTH) << x[i] << " : " << std::setw(WIDTH) << y[i] << " : " << std::setw(WIDTH) << y[i]/x[i] << '\n';
    }

    std::cout << fp32_matmul << '\n';




    return EXIT_SUCCESS;
}

/*
  with speed of light
  latency (sec) :     energy (J)  :      power (W)
       7.68e-07 :       0.0014068 :         1831.76
       7.68e-07 :     0.000837628 :         1090.66
       7.68e-07 :     0.000553045 :          720.11
  with DRAM latency constraints
  latency (sec) :     energy (J)  :      power (W)
    1.36567e-05 :       0.0014068 :         103.011
       6.83e-06 :     0.000837628 :         122.640
    3.41667e-06 :     0.000553045 :         161.867

Compute Event Counts:
  Op: INSTR_FETCH, Width: 32, Count: 16777216
  Op: INSTR_DECODE, Width: 32, Count: 16777216
  Op: INSTR_DISPATCH, Width: 32, Count: 16777216
  Op: ALU_FMA, Width: 32, Count: 16777216

Memory Event Counts:
  Op: RF_READ, Width: 32, Burst Length: 0, Count: 50331648
  Op: RF_WRITE, Width: 32, Burst Length: 0, Count: 16777216
  Op: DRAM_READ, Width: 64, Burst Length: 8, Count: 8194
  Op: DRAM_WRITE, Width: 64, Burst Length: 8, Count: 4097
  
*/