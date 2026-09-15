#include "keel/engine.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

// Synthetic two-sided flow around a drifting fair value.
// Measures match-path latency in nanoseconds. This is the artifact.

namespace {
uint32_t rng_state = 0xC001D00D;
uint32_t rng() {
    rng_state += 0x6D2B79F5u;
    uint32_t t = (rng_state ^ (rng_state >> 15)) * (1u | rng_state);
    t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
    return t ^ (t >> 14);
}
double urand() { return rng() / 4294967296.0; }
int irand(int a, int b) { return a + int(urand() * (b - a + 1)); }
}  // namespace

int main() {
    using clock = std::chrono::steady_clock;
    keel::Engine<1 << 16, 4096> eng;
    const int base = 8000;
    eng.set_base(base);

    // Seed a book: 20 ticks each side around 10000.
    const int fair0 = 10000;
    for (int d = 1; d <= 24; ++d) {
        eng.on_limit(keel::Side::Buy, fair0 - d, 50 + irand(0, 80));
        eng.on_limit(keel::Side::Sell, fair0 + d, 50 + irand(0, 80));
    }

    constexpr int N = 400000;
    std::vector<uint64_t> ns;
    ns.reserve(N);

    int fair = fair0;
    uint32_t live_buy[64]{};
    uint32_t live_sell[64]{};
    int nb = 0, nsell = 0;

    auto t0 = clock::now();
    for (int i = 0; i < N; ++i) {
        if ((i & 31) == 0) fair += irand(-1, 1);
        if (fair < base + 40) fair = base + 40;
        if (fair > base + 4000 - 40) fair = base + 4000 - 40;

        double u = urand();
        auto t1 = clock::now();

        if (u < 0.12) {
            keel::Side s = urand() < 0.5 ? keel::Side::Buy : keel::Side::Sell;
            eng.on_market(s, irand(5, 40));
        } else if (u < 0.22 && (nb + nsell) > 8) {
            if (urand() < 0.5 && nb) {
                eng.on_cancel(live_buy[--nb]);
            } else if (nsell) {
                eng.on_cancel(live_sell[--nsell]);
            }
        } else {
            keel::Side s = urand() < 0.5 ? keel::Side::Buy : keel::Side::Sell;
            int px = s == keel::Side::Buy ? fair - irand(1, 12) : fair + irand(1, 12);
            uint32_t id = eng.on_limit(s, px, irand(10, 90));
            if (id) {
                if (s == keel::Side::Buy && nb < 64) live_buy[nb++] = id;
                if (s == keel::Side::Sell && nsell < 64) live_sell[nsell++] = id;
            }
        }

        auto t2 = clock::now();
        ns.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count());
    }
    auto t3 = clock::now();

    std::sort(ns.begin(), ns.end());
    auto pct = [&](double p) {
        return ns[std::min(ns.size() - 1, size_t(p * (ns.size() - 1)))];
    };

    double sec = std::chrono::duration<double>(t3 - t0).count();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "KEEL matching engine\n";
    std::cout << "events     " << N << "\n";
    std::cout << "elapsed    " << sec << " s\n";
    std::cout << "rate       " << (N / sec) << " msg/s\n";
    std::cout << "fills      " << eng.stats.fills << "  qty " << eng.stats.fill_qty << "\n";
    std::cout << "live ords  " << eng.pool.live << "\n";
    std::cout << "best       " << eng.bid_px() << " x " << eng.ask_px()
              << "  spread " << eng.spread_ticks() << " ticks\n";
    std::cout << "microprice " << eng.microprice() << "\n";
    std::cout << "latency ns p50=" << pct(0.50)
              << "  p90=" << pct(0.90)
              << "  p99=" << pct(0.99)
              << "  p999=" << pct(0.999)
              << "  max=" << ns.back() << "\n";

    std::vector<keel::Engine<1 << 16, 4096>::L2> b, a;
    eng.snapshot(b, a, 8);
    std::cout << "\nL2\n";
    for (int i = int(a.size()) - 1; i >= 0; --i)
        std::cout << "  ask " << a[i].price << "  " << a[i].qty << "  n=" << a[i].n_orders << "\n";
    std::cout << "  ---\n";
    for (auto& x : b)
        std::cout << "  bid " << x.price << "  " << x.qty << "  n=" << x.n_orders << "\n";
    return 0;
}
