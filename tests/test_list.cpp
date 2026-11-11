#if __has_include("../runtime/List.h")
#include "../runtime/List.h"
#else
#include "List.h"
#endif

#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using gash::List;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Exception, typename Action>
void requireThrows(Action action, const std::string& message) {
    try { action(); }
    catch (const Exception&) { return; }
    throw std::runtime_error(message);
}

struct Test { const char* name; std::function<void()> fn; };

int main() {
    const std::vector<Test> tests = {
        {"A4-01 initial size", [] { List<int> a(3); require(a.size() == 3, "size"); }},
        {"A4-02 initial capacity", [] { List<int> a(3); require(a.capacity() >= 3, "capacity"); }},
        {"A4-03 slots start uninitialized", [] { List<int> a(3); for (int i=0;i<3;++i) require(!a.isInitialized(i), "occupancy"); }},
        {"A4-04 get uninitialized rejects", [] { List<int> a(1); requireThrows<std::runtime_error>([&] { (void)a.get(0); }, "read should fail"); }},
        {"A4-05 set then get", [] { List<int> a(2); a.set(0, 42); require(a.get(0) == 42 && a.isInitialized(0), "write/read"); }},
        {"A4-06 separate occupancy", [] { List<int> a(2); a.set(1, 8); require(!a.isInitialized(0) && a.isInitialized(1), "occupancy"); }},
        {"A4-07 negative size rejects", [] { requireThrows<std::length_error>([] { List<int> a(-1); }, "negative length"); }},
        {"A4-08 negative get index rejects", [] { List<int> a(1); requireThrows<std::out_of_range>([&] { (void)a.get(-1); }, "negative index"); }},
        {"A4-09 high get index rejects", [] { List<int> a(1); requireThrows<std::out_of_range>([&] { (void)a.get(1); }, "high index"); }},
        {"A4-10 high set index rejects", [] { List<int> a(1); requireThrows<std::out_of_range>([&] { a.set(1, 3); }, "high index"); }},
        {"A4-11 invalid occupancy index rejects", [] { List<int> a(1); requireThrows<std::out_of_range>([&] { (void)a.isInitialized(-1); }, "occupancy bounds"); }},
        {"A4-12 shrink preserves survivors", [] { List<int> a(3); a.set(0, 1); a.set(1, 2); a.set(2, 3); a.resize(2); require(a.size() == 2 && a.get(0) == 1 && a.get(1) == 2, "shrink"); }},
        {"A4-13 shrink keeps capacity", [] { List<int> a(3); auto c = a.capacity(); a.resize(1); require(a.capacity() == c, "capacity shrink"); }},
        {"A4-14 regrow clears truncated slot", [] { List<int> a(3); a.set(2, 99); a.resize(2); a.resize(3); require(!a.isInitialized(2), "stale value resurrected"); }},
        {"A4-15 grow adds empty slots", [] { List<int> a(1); a.set(0, 5); a.resize(4); require(a.get(0) == 5 && !a.isInitialized(1) && !a.isInitialized(3), "growth"); }},
        {"A4-16 negative resize rejects without changing", [] { List<int> a(2); requireThrows<std::length_error>([&] { a.resize(-1); }, "negative resize"); require(a.size() == 2, "modified on failure"); }},
        {"A4-17 copy is independent", [] { List<std::string> a(2); a.set(0, "old"); auto b = a; b.set(0, "new"); b.set(1, "extra"); require(a.get(0) == "old" && !a.isInitialized(1), "shared state"); }},
        {"A4-18 assignment is independent", [] { List<int> a(2); a.set(0, 7); List<int> b(0); b = a; b.set(0, 9); require(a.get(0) == 7 && b.get(0) == 9, "copy assignment"); }},
        {"A4-19 empty list", [] { List<int> a(0); require(a.size() == 0, "empty"); requireThrows<std::out_of_range>([&] { a.set(0, 1); }, "empty bounds"); }},
        {"A4-20 multiple supported types", [] { List<double> d(1); d.set(0, 2.5); List<bool> b(1); b.set(0, true); List<char> c(1); c.set(0, 'z'); require(d.get(0) == 2.5 && b.get(0) && c.get(0) == 'z', "type support"); }}
    };
    std::size_t passed = 0;
    for (const auto& t : tests) {
        try { t.fn(); std::cout << "[PASS] " << t.name << '\n'; ++passed; }
        catch (const std::exception& e) { std::cout << "[FAIL] " << t.name << ": " << e.what() << '\n'; }
    }
    std::cout << "Result: " << passed << "/" << tests.size() << " tests passed\n";
    return passed == tests.size() ? 0 : 1;
}
