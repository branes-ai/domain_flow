# Cache Transactions

L1 cache access size and burst behavior:

The size of an L1 cache access is typically a **cache line**, which is a fixed-size block of memory. The most common cache line size in modern x86 and ARM microprocessors is **64 bytes**. However, some architectures might use 32 or 128-byte cache lines.

When the CPU needs data from memory, it doesn't request individual bytes. Instead, the memory controller fetches an entire cache line containing the requested data. This is done for several reasons:

* **Spatial Locality:** If the CPU accesses one memory location, it's likely to access nearby locations soon. Fetching a whole line brings in this neighboring data, increasing the chance of a cache hit for subsequent accesses.
* **Efficiency:** Transferring data in larger blocks is generally more efficient than transferring many small individual pieces due to the overhead associated with each transfer.

Therefore, an L1 cache access is **typically done in one fetch** of the entire cache line (e.g., 64 bytes). It's not usually a burst of smaller packets sent or received for a single L1 cache access. The memory system is optimized to transfer these fixed-size cache lines as a contiguous block.

**In summary:**

* **Access Size:** Typically a full cache line (most commonly 64 bytes).
* **Fetch Behavior:** Usually a single, contiguous block transfer, not a burst of smaller packets for one access.

It's important to note that while a single access usually fetches a full line, modern processors employ various techniques like prefetching to bring additional cache lines into the L1 cache in anticipation of future needs. These prefetches might occur as separate transfers, but a single data request from the CPU to the L1 cache that results in a miss will generally trigger a single fetch of the relevant cache line from the next level of the memory hierarchy (L2 cache, main memory, etc.).

# L2 and L3 Caches

Let's break down the L2 and L3 cache behavior in multi-core systems:

**L2 Cache in Multi-Core Configurations:**

* **Access Size:** Similar to L1, the L2 cache also operates on **cache lines**. While the most common size is 64 bytes, some architectures might employ different sizes (e.g., 128 bytes). So, a single fetch from L2 will typically be a full cache line.
* **Bus Width:** The internal bus width between the CPU cores and their private L2 caches (in architectures where L2 is per-core) is generally quite wide to facilitate fast data transfer. A **512-bit (64-byte)** wide bus would not be unusual, and some high-performance cores might even have wider internal buses to their L2 caches. This allows an entire 64-byte cache line to be transferred in a single cycle (or a few cycles depending on the clock speed and architecture).
* **Fetch Behavior:** Access to a private L2 cache (if the data isn't in L1) will typically involve fetching the entire cache line in one operation over this wide internal bus. It's not generally packetized.

**L3 Cache (SoC-wide Memory Buffer for DRAM):**

* **Access Size:** The L3 cache also deals with **cache lines**, and the size will typically be consistent with the L1 and L2 caches within the same architecture (e.g., 64 bytes).
* **Bus Width:** The bus connecting the CPU cores (or core complexes) to the shared L3 cache can be very wide, often **512 bits (64 bytes) or even wider** in high-performance SoCs. This wide bus allows for efficient transfer of cache lines between the cores and the L3.
* **Fetch Behavior:** When a core needs data that is in the L3 cache (but not L1 or L2), the entire cache line is usually transferred over the wide internal bus in a single, contiguous operation.

**Packetization Considerations:**

While the transfer of a single cache line between cache levels (L1 to L2, L2 to L3) is generally a single, wide burst, there are scenarios where data movement might appear more "packetized" at a higher level:

* **Cache Coherency Traffic:** In multi-core systems, maintaining cache coherency (ensuring all cores have a consistent view of memory) involves sending messages or "snoops" between cores and caches. These coherency messages might be smaller than a full cache line and could be considered a form of packetized communication related to memory access.
* **Interconnect Fabrics:** Modern SoCs use complex interconnect fabrics (like on-chip networks) to connect various components, including CPU cores, GPUs, and the memory controller. While the fundamental unit of transfer for cache access might still be a cache line, the routing and arbitration within these interconnects might involve breaking down larger transfers into smaller flits (flow control units) for efficient routing. However, for a single cache line request, the L3 cache would ideally respond with the entire line in a burst over a wide portion of this interconnect.
* **Memory Controller Interface:** The interface between the L3 cache and the DRAM memory controller will also have a certain width (e.g., 64-bit, 128-bit, or wider memory channels). When the L3 needs to fetch a cache line from DRAM, this transfer will occur over the memory bus. While the cache line is the unit of transfer from the L3's perspective, the actual DRAM access might involve multiple smaller bursts depending on the DRAM organization and the memory controller's operation. However, this is at the DRAM level, not typically seen as packetization between the L3 and the core.

## Summary

* L1 cache access is a full cache line (most commonly 64 bytes), fetched in a single, contiguous block transfer over a 512b wide bus.
* L2 cache access in multi-core systems typically involves fetching a full cache line (usually 64 bytes) over a wide internal bus (potentially 512 bits or more) in a single operation.
* L3 cache access also deals with cache lines (typically the same size as L1/L2) and utilizes a wide on-chip bus (often 512 bits or wider) for efficient, single-burst transfers to the requesting core.
* While cache coherency traffic and the internal workings of the SoC interconnect might involve smaller packets, the fundamental data transfer unit for cache-to-cache or cache-to-core communication for a single access remains the cache line transferred in a burst over a wide bus. The DRAM interface might see the cache line broken down into smaller transfers at the memory controller level.