#pragma once
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <array>
#include <cassert>

namespace warlock {

enum class EventType : uint8_t {
    NONE = 0,
    CAST_FINISH,        // Cast completes, spell fires (or starts missile)
    SPELL_IMPACT,       // Projectile reaches boss, damage and debuffs resolve
    DOT_TICK,           // Periodic damage tick (Corruption, Immolate, Agony)
    CHANNEL_TICK,       // Channeled spell tick (Drain Life)
    GCD_READY,          // Global Cooldown finishes
    COOLDOWN_READY,     // Ability or trinket cooldown finishes
    BUFF_EXPIRE,        // Temporary buff drops (trinket, potion, Shadow Trance)
    DEBUFF_EXPIRE,      // Boss debuff drops (ISB, Curse)
    MANA_REGEN_TICK,    // 2-second or 5-second mana tick
    PET_MELEE_SWING,    // Pet auto-attack melee swing (every 2.0s)
    PET_CAST_FINISH,    // Pet special ability (Succubus Lash of Pain / Imp Firebolt)
    SIMULATION_END      // Force end of simulation
};

inline const char* event_type_to_string(EventType t) {
    switch (t) {
        case EventType::CAST_FINISH: return "Cast Finish";
        case EventType::SPELL_IMPACT: return "Spell Impact";
        case EventType::DOT_TICK: return "DoT Tick";
        case EventType::CHANNEL_TICK: return "Channel Tick";
        case EventType::GCD_READY: return "GCD Ready";
        case EventType::COOLDOWN_READY: return "Cooldown Ready";
        case EventType::BUFF_EXPIRE: return "Buff Expire";
        case EventType::DEBUFF_EXPIRE: return "Debuff Expire";
        case EventType::MANA_REGEN_TICK: return "Mana Regen Tick";
        case EventType::PET_CAST_FINISH: return "Pet Cast Finish";
        case EventType::SIMULATION_END: return "Simulation End";
        default: return "None";
    }
}

// Fixed-size POD event structure
struct Event {
    double time = 0.0;
    EventType type = EventType::NONE;
    uint8_t spell_id = 0;       // E.g. SpellID::SHADOW_BOLT
    uint16_t sub_id = 0;        // E.g. tick number, buff ID, etc.
    uint32_t user_data = 0;
    double float_data = 0.0;    // E.g. snapshotted spell damage or tick damage

    bool operator>(const Event& other) const {
        return time > other.time;
    }
    bool operator<(const Event& other) const {
        return time < other.time;
    }
};

// Ultra-fast zero-allocation binary min-heap for DES
template <size_t Capacity = 256>
class FastEventQueue {
private:
    std::array<Event, Capacity> heap_;
    size_t size_ = 0;

    inline void sift_up(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (heap_[index].time < heap_[parent].time) {
                std::swap(heap_[index], heap_[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    inline void sift_down(size_t index) {
        while (true) {
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            size_t smallest = index;

            if (left < size_ && heap_[left].time < heap_[smallest].time) {
                smallest = left;
            }
            if (right < size_ && heap_[right].time < heap_[smallest].time) {
                smallest = right;
            }

            if (smallest != index) {
                std::swap(heap_[index], heap_[smallest]);
                index = smallest;
            } else {
                break;
            }
        }
    }

public:
    FastEventQueue() : size_(0) {}

    inline void clear() {
        size_ = 0;
    }

    inline bool empty() const {
        return size_ == 0;
    }

    inline size_t size() const {
        return size_;
    }

    inline bool push(const Event& event) {
        if (size_ >= Capacity) {
            return false; // Heap full guard
        }
        heap_[size_] = event;
        sift_up(size_);
        size_++;
        return true;
    }

    inline bool push(double time, EventType type, uint8_t spell_id = 0, uint16_t sub_id = 0, uint32_t user_data = 0, double float_data = 0.0) {
        Event ev{time, type, spell_id, sub_id, user_data, float_data};
        return push(ev);
    }

    inline Event peek() const {
        return heap_[0];
    }

    inline Event pop() {
        assert(size_ > 0);
        Event top = heap_[0];
        size_--;
        if (size_ > 0) {
            heap_[0] = heap_[size_];
            sift_down(0);
        }
        return top;
    }
};

// Fast non-cryptographic PRNG (xoshiro256** for extreme speed and thread isolation)
class FastRNG {
private:
    uint64_t s[4];

    static inline uint64_t rotl(const uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

    static inline uint64_t splitmix64(uint64_t& x) {
        uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

public:
    explicit FastRNG(uint64_t seed = 1337ULL) {
        uint64_t sm = seed ? seed : 0xDEADBEEFCAFEULL;
        s[0] = splitmix64(sm);
        s[1] = splitmix64(sm);
        s[2] = splitmix64(sm);
        s[3] = splitmix64(sm);
    }

    inline uint64_t next_u64() {
        const uint64_t result = rotl(s[1] * 5, 7) * 9;
        const uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;
        s[3] = rotl(s[3], 45);

        return result;
    }

    // Returns a uniform float in [0.0, 1.0)
    inline double next_double() {
        return (next_u64() >> 11) * (1.0 / 9007199254740992.0);
    }

    // Returns random double in [min, max]
    inline double range(double min, double max) {
        return min + next_double() * (max - min);
    }

    // True with probability p
    inline bool chance(double p) {
        return next_double() < p;
    }
};

} // namespace warlock
