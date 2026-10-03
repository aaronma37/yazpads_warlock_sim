# Warlock Simulation Throughput & Scaling Benchmark Report

- **Environment**: `Linux x86_64` (8 CPU cores)
- **Date / Time**: `2026-10-02 22:53:16 UTC`
- **Single-Core CPU Baseline**: `2,542 fights/s`
- **Multi-Core CPU (8T) Baseline**: `7,690 fights/s`

| Batch / Workload | Total Fights | GPU Latency | GPU Throughput | Speedup vs 1-CPU | Speedup vs Multi-CPU |
|---|---|---|---|---|---|
| `Batch_1000` | 1,000 | 73.5 ms | **13,605 fights/s** | `5.4x` | `1.8x` |
| `Batch_10000` | 10,000 | 120.3 ms | **83,126 fights/s** | `32.7x` | `10.8x` |
| `Batch_50000` | 50,000 | 425.7 ms | **117,454 fights/s** | `46.2x` | `15.3x` |
| `Batch_100000` | 100,000 | 810.0 ms | **123,457 fights/s** | `48.6x` | `16.1x` |
| `Batch_500000` | 500,000 | 2.91 s | **171,597 fights/s** | `67.5x` | `22.3x` |
| `Batch_1000000` | 1,000,000 | 5.44 s | **183,719 fights/s** | `72.3x` | `23.9x` |
| `MultiSpec_22x1000` | 22,000 | 362.5 ms | **60,690 fights/s** | `23.9x` | `7.9x` |
| `MultiSpec_22x5000` | 110,000 | 1.21 s | **91,120 fights/s** | `35.8x` | `11.8x` |
| `MultiSpec_22x10000` | 220,000 | 2.02 s | **109,062 fights/s** | `42.9x` | `14.2x` |
