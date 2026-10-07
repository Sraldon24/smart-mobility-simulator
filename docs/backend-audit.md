# Backend Engineering Audit & Performance Baseline

This document captures the state of the Smart Mobility Simulator backend following MVP 7 Step 4 (Engineering Cleanup). It identifies technical debt, maintainability concerns, and provides measured baseline performance data.

*Note: No major speculative optimizations have been applied yet. Step 5 will address the performance bottlenecks identified here.*

## 1. Measured Performance Baseline

All measurements taken on a Linux Release build using `utils::ScopedTimer`.

| Component / Subsystem | Scenario | Measured Time | Notes |
| :--- | :--- | :--- | :--- |
| **OSM Importer** | Montreal `montreal.osm.pbf` (startup) | ~4164 ms | Heavy DOM parsing; synchronous load at startup. |
| **Routing (Dijkstra)** | `0 -> 24` (800m, Fastest) | ~27 us (first run) / ~14 us (subsequent) | |
| **Routing (A*)** | `0 -> 24` (800m, Fastest) | ~15 us | |
| **Simulation Engine** | `update(1.0)` with 10 vehicles | ~4 us | |
| **Simulation Engine** | `update(1.0)` with 50 vehicles | ~10 us | Linear scaling. |
| **Simulation Engine** | `update(1.0)` with 100 vehicles | ~13 us | Linear scaling. |
| **Candidate Generation** | 2 candidates for `0 -> 24` | ~68 us | |

## 2. Technical Debt & Audit Findings

### A. Correctness Risks
- Unchecked node and road IDs in `SimulationEngine` when updating vehicles. If an incident deletes a road out from under a vehicle, the simulation might crash or vehicle might get stuck.
- Pointers returned from `RoadNetwork::getRoad()` and `getNodeById()` are currently assumed non-null by several callers.

### B. Maintainability Problems
- The `Router` class has duplicate queue logic for Dijkstra and A*.
- The `HttpServer` has very large lambda handler functions that mix routing logic, response formatting, and validation.

### C. Duplicated Logic
- Routing functions have copy-pasted `visited` set and priority queue code.

### D. Missing Diagnostics & Profiling
- Solved during this step. Added `utils::Logger` and `utils::ScopedTimer`.

### E. Portability Concerns
- File paths for data (e.g., `montreal.osm.pbf`) rely on working directory conventions (`../data/...`). Hardcoded strings should be replaced by configuration or command-line flags.

## 3. Recommended Optimization Candidates (For MVP 7 Step 5)

Based strictly on the measured baseline:

1. **OSM Data Ingestion (High Impact)**: The 4+ second load time for Montreal is very noticeable on startup. The data could be pre-processed into a binary cache or loaded asynchronously.
2. **Graph Memory Layout**: The A*/Dijkstra algorithms take 15-30 us for tiny routes. As the graph scales up, `std::unordered_map` and pointer-chasing will become cache-unfriendly. Flattening the adjacency list into a contiguous structure (e.g., CSR) is recommended.
3. **HTTP Server Handlers**: Though not a raw CPU bottleneck, splitting handlers out of `main.cpp` or `HttpServer::listen` will significantly improve compile times.

