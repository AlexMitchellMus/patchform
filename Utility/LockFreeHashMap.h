/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

// This is a lock free hash map, based on: https://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table/

// We want to save strings that are used in the graph, but we don't want to allocate on the audio thread.
// Even if most are small, (and hence small string) we still want to be able to save out the string or know what it is at some point
// We could save the string where we use it (in events, and data atoms - save both hash, and string)
// However then we will be duplicating strings each time
// So the solution is to make events/atoms only hold the strings hash
// This way we can fast compare the strings in switches
// We hold all strings in a fixed size array (32768 possible unique strings)
// When an object needs to retrieve a string throughout the system, it should first intern the string.
// The hashmap is designed to be safe to access from multiple threads
// To do this, we use a fixed size atomic hash map, and we do not allow removing strings/hashes from the map
// This also has no fallback for collisions - if something collides (we will have to test for this!) use another string

#pragma once

#include <array>
#include <atomic>
#include <string>
#include <cstdint>

#define LOCKFREE_HASHMAP_ENABLE_LOG_TIME 1

#if LOCKFREE_HASHMAP_ENABLE_LOG_TIME
  #include <chrono>
  #define LOG_TIME_START auto __log_start = std::chrono::high_resolution_clock::now();
  #define LOG_TIME(name) log_duration(__log_start, name);
#else
  #define LOG_TIME_START
  #define LOG_TIME(name)
#endif

struct Entry {
    std::atomic<hash32> key{0};
    std::string* value = nullptr; // not atomic, safe after key is published - if key is not published, we can't access it anyway!
};

class LockFreeHashMap {
public:
    // Use a table size which is a power of two, so we don't need to use modulo for: size_t pos = (index + i) & TABLE_MASK
    static constexpr size_t TABLE_SIZE = 1 << 15; // 32768, power of 2
    static constexpr size_t TABLE_MASK = TABLE_SIZE - 1;

    LockFreeHashMap() {
        value_pool.fill("");
    }

    template <typename... Strings>
    void intern(Strings&&... strings) {
        (internString(std::forward<Strings>(strings)), ...);
    }

    void internString(const std::string& value)
    {
        intern(hash(value), value);
    }

    bool intern(const hash32 key, const std::string& value) {
        const size_t index = key & TABLE_MASK;
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            size_t pos = (index + i) & TABLE_MASK;

            hash32 current = entries[pos].key.load(std::memory_order_relaxed);
            if (current == 0) {
                hash32 expected = 0;
                if (entries[pos].key.compare_exchange_strong(expected, key, std::memory_order_acq_rel)) {
                    size_t value_slot = value_index.fetch_add(1, std::memory_order_relaxed);
                    if (value_slot >= TABLE_SIZE) return false;

                    value_pool[value_slot] = value;
                    entries[pos].value = &value_pool[value_slot];
                    return true;
                }
            } else if (current == key) {
                return true; // already interned
            }
        }
        return false; // table full
    }


    std::string* find(const hash32 key) const {
        LOG_TIME_START

        const size_t index = key & TABLE_MASK;
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            const size_t pos = (index + i) & TABLE_MASK;

            hash32 k = entries[pos].key.load(std::memory_order_relaxed);
            if (k == 0) {
                LOG_TIME("find (miss)")
                return nullptr;
            }
            if (k == key) {
                LOG_TIME("find (hit)")
                return entries[pos].value;
            }
        }

        LOG_TIME("find (fallback)")
        return nullptr;
    }


private:
    std::array<Entry, TABLE_SIZE> entries;
    std::array<std::string, TABLE_SIZE> value_pool;
    std::atomic<size_t> value_index{0};

#if LOCKFREE_HASHMAP_ENABLE_LOG_TIME
    void log_duration(std::chrono::high_resolution_clock::time_point start, const char* label) const {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        std::cout << label << " took " << duration.count() << " ns\n";
    }
#endif
};
