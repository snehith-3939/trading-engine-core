# Trading Engine Core

> A high-performance, in-memory order matching engine implemented in C++17.
> Built to understand how real electronic exchanges process millions of orders with sub-microsecond latency.

---

## Performance

| Metric | Result |
|---|---|
| Orders Processed | 5,000,000 |
| Total Time | 1,076 ms |
| Throughput | **4.64 Million orders/sec** |
| Avg Latency | **215 ns / order** |

Tested on Apple Silicon M4, compiled with `clang++ -O3`.

---

## What This Is

Modern stock exchanges like NSE or NYSE need to match buy and sell orders in microseconds. The piece of software that does this is called a **Matching Engine**. This project is a from-scratch implementation of one.

It maintains a live order book, matches incoming orders against resting liquidity in strict price-time priority, and handles complex order types like Stop and Stop-Limit orders. The design prioritizes cache efficiency and memory predictability over simplicity.

---

## Architecture

```
Trading-Engine-Core/
├── include/
│   ├── MemoryArena.hpp      # Custom memory pool (no heap allocations at runtime)
│   ├── TradeTicket.hpp      # A single order (node in a linked list)
│   ├── PriceLevel.hpp       # A price bucket (node in an AVL tree)
│   └── MatchingEngine.hpp   # The top-level engine
├── src/
│   ├── TradeTicket.cpp
│   ├── PriceLevel.cpp
│   ├── MatchingEngine.cpp
│   ├── OrderGenerator.cpp   # Randomized order flow generator for benchmarking
│   └── main.cpp             # Entry point and benchmark runner
└── README.md
```

---

## Core Components

### MemoryArena

Instead of allocating each order individually on the heap with `new`, the engine uses a custom templated memory pool. At startup, it reserves a large contiguous block of memory and serves objects from it directly. This eliminates heap fragmentation, makes allocation O(1), and keeps memory access patterns cache-friendly.

### TradeTicket

Represents a single order. Orders at the same price level are stored as nodes in a **doubly-linked list**, which gives O(1) insertion at the tail and O(1) cancellation at any position without searching the entire list.

### PriceLevel

A price bucket holding all orders at one specific price. Price levels are organized as nodes in a **self-balancing AVL tree**, keeping all levels sorted by price at all times. This means the best bid and best ask are always at the leftmost and rightmost nodes of their respective trees, accessible in O(log n).

### MatchingEngine

The orchestrator. Maintains four separate AVL trees:

| Tree | Purpose |
|---|---|
| Bid Tree | Buy limit orders, sorted descending by price |
| Ask Tree | Sell limit orders, sorted ascending by price |
| Stop-Bid Tree | Buy stop orders awaiting trigger |
| Stop-Ask Tree | Sell stop orders awaiting trigger |

When a market order arrives, the engine walks the opposite side of the book and fills resting orders in strict price-time priority until the incoming quantity is exhausted.

---

## Supported Order Types

| Order Type | Description |
|---|---|
| **Market** | Executes immediately at the best available price |
| **Limit** | Rests in the book at a specified price, waits for a match |
| **Stop** | Converts to a market order when a trigger price is crossed |
| **Stop-Limit** | Converts to a limit order when a trigger price is crossed |

All four order types support full **cancel** and **modify** operations.

---

## Key Design Decisions

**Why AVL trees instead of a sorted array or hash map?**

A hash map gives O(1) price-level lookup but has no concept of ordering. To find the best bid or ask, you would need to scan the entire map every time, which is O(n). A sorted array gives O(1) access to the best price but O(n) insertion when a new price level appears. An AVL tree gives O(log n) for all operations (insert, delete, find-min, find-max) while keeping everything sorted at all times.

**Why integer prices?**

All prices are stored in the smallest currency unit (paise) as integers rather than floating-point values. Floating-point arithmetic introduces rounding errors that can produce incorrect order matching in edge cases. Integer comparisons are also faster and fully deterministic.

**Why a custom MemoryArena instead of std::allocator?**

Under high load, repeated calls to `new` and `delete` become a measurable bottleneck due to heap fragmentation and the overhead of the allocator's internal bookkeeping. The custom arena eliminates all of this. Every `TradeTicket` and `PriceLevel` is acquired from a pre-allocated pool and returned to it when done, keeping allocations at a constant cost.

---

## Build and Run

Requires a C++17 compiler. Tested with `clang++` on macOS.

```bash
# Compile with maximum optimization
clang++ -I include -O3 -std=c++17 src/*.cpp -o TradingEngine

# Run the benchmark
./TradingEngine
```

Expected output:

```
Orders: 5000000
Time: 1076 ms
TPS: 4644652 orders/sec
Avg latency: 215 ns/order
```

---

## What I Learned

The biggest takeaway from this project was understanding how much the choice of data structure matters when performance constraints are real rather than theoretical.

The first version of this engine used `std::map` for price levels and `std::allocator` for order objects. It was correct but slow. Replacing the map with a hand-written AVL tree, switching to a linked-list queue within each level, and introducing the memory arena brought latency down by over 3x.

Writing AVL rotations by hand and debugging pointer errors in the linked list was genuinely difficult, but it gave me a much clearer picture of how real systems are designed when every nanosecond counts.

---

## Note

This repository is the public release of a project I developed and tested locally before migrating to GitHub for my portfolio.
