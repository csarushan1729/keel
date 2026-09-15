# KEEL

Low-latency **price-time matching engine** and L3 limit order book in C++17.

This is a systems piece, not a web app and not a pairs-trading notebook.

## What it is

- Integer-tick L3 book. Each price level is an intrusive FIFO of resting orders.
- Orders live in a **fixed pool**. After warmup there is no `new` on the match path.
- Limit, market, cancel. Price-time priority. Partial fills.
- Lock-free **SPSC ring** for inbound messages (single feeder thread → matcher).
- Top-of-book **microprice** and spread; snapshot for L2 depth.
- Built-in replay: synthetic maker/taker flow, nanosecond latency histogram.

## Build (Windows Command Prompt)

Install [Git](https://git-scm.com/download/win) and [MSYS2](https://www.msys2.org/) first. In an **MSYS2 UCRT64** terminal:

```bat
pacman -S --needed mingw-w64-ucrt-x86_64-gcc
g++ -std=c++17 -O3 -I include src/main.cpp -o keel.exe
keel.exe
```

Or with Visual Studio Build Tools (Developer Command Prompt):

```bat
cl /std:c++17 /O2 /EHsc /I include src\main.cpp /Fe:keel.exe
keel.exe
```

Expected: ~hundreds of thousands of events, latency printed in nanoseconds (p50 / p90 / p99 / p999).

## Build (Linux / macOS)

```bash
g++ -std=c++17 -O3 -I include src/main.cpp -o keel
./keel
```

or

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/keel
```

## Resume line

> C++17 price-time matching engine with a pooled L3 book (no heap on the match path), SPSC ingress, and a nanosecond latency histogram under synthetic maker/taker flow.

## Files

| File | Role |
|---|---|
| `include/keel/engine.hpp` | Pool, ladder, matcher, SPSC |
| `src/main.cpp` | 400k-event replay + p50/p99/p999 |
