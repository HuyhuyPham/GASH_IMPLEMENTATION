
#if __has_include("../runtime/List.h")
#include "../runtime/List.h"
#else
#include "List.h"
#endif

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

// Tracks the lifetime of objects stored inside lists.
struct Tracked {
    static int live;
    static int destroyed;

    int value;

    explicit Tracked(int v) : value(v) {
        ++live;
    }

    Tracked(const Tracked& other)
        : value(other.value) {
        ++live;
    }

    Tracked(Tracked&& other) noexcept
        : value(other.value) {
        ++live;
    }

    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) = default;

    ~Tracked() {
        --live;
        ++destroyed;
    }
};

int Tracked::live = 0;
int Tracked::destroyed = 0;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename F>
void run(const char* name, F test, int& passed) {
    require(
        Tracked::live == 0,
        "Test started with live tracked objects"
    );

    test();

    require(
        Tracked::live == 0,
        "Test leaked tracked objects"
    );

    ++passed;
    std::cout << "[PASS] " << name << '\n';
}

} // namespace

int main() {
    int passed = 0;

    try {
        // Test 1: Empty slots need no manual cleanup.
        run("Empty list requires no manual cleanup", [] {
            {
                gash::List<Tracked> items(3);

                require(items.size() == 3, "Wrong size");
                require(
                    Tracked::live == 0,
                    "Uninitialized slots constructed objects"
                );
            }
        }, passed);

        // Test 2: Initialized elements are destroyed
        // automatically at scope exit.
        run("Initialized element released at scope exit", [] {
            {
                gash::List<Tracked> items(1);

                {
                    Tracked value(7);
                    items.set(0, value);

                    require(
                        Tracked::live == 2,
                        "Expected two live objects"
                    );
                }

                require(
                    Tracked::live == 1,
                    "Stored item missing"
                );
            }

            require(
                Tracked::live == 0,
                "List element not destroyed"
            );
        }, passed);

        // Test 3: Nested lists clean up independently.
        run("Multiple lists clean up independently", [] {
            {
                gash::List<Tracked> first(1);

                {
                    Tracked value(1);
                    first.set(0, value);
                }

                {
                    gash::List<Tracked> second(1);

                    {
                        Tracked value(2);
                        second.set(0, value);
                    }

                    require(
                        Tracked::live == 2,
                        "Two list elements expected"
                    );
                }

                require(
                    Tracked::live == 1,
                    "Inner list was not destroyed"
                );
            }
        }, passed);

        // Test 4: Shrinking destroys truncated elements.
        run("Shrinking destroys discarded elements", [] {
            gash::List<Tracked> items(3);

            for (int i = 0; i < 3; ++i) {
                Tracked value(i);
                items.set(i, value);
            }

            require(
                Tracked::live == 3,
                "Three elements expected"
            );

            items.resize(1);

            require(
                Tracked::live == 1,
                "Removed elements still alive"
            );

            require(
                items.get(0).value == 0,
                "Retained value changed"
            );
        }, passed);

        // Test 5: Regrowing does not restore removed values.
        run("Regrowing leaves discarded slots empty", [] {
            gash::List<Tracked> items(2);

            {
                Tracked value(4);
                items.set(1, value);
            }

            items.resize(1);
            items.resize(2);

            require(
                !items.isInitialized(1),
                "Discarded element reappeared"
            );

            require(
                Tracked::live == 0,
                "Discarded element remains alive"
            );
        }, passed);

        // Test 6: List copies own independent elements.
        run("Copies own independent elements", [] {
            gash::List<Tracked> original(1);

            {
                Tracked value(10);
                original.set(0, value);
            }

            {
                gash::List<Tracked> copy = original;

                require(
                    Tracked::live == 2,
                    "Copy did not duplicate the value"
                );

                require(
                    copy.get(0).value == 10,
                    "Copy has wrong value"
                );
            }

            require(
                Tracked::live == 1,
                "Copy destruction affected original"
            );

            require(
                original.get(0).value == 10,
                "Original was invalidated"
            );
        }, passed);

        // Test 7: Moving transfers ownership.
        run("Moved list cleans up exactly once", [] {
            {
                gash::List<Tracked> source(1);

                {
                    Tracked value(11);
                    source.set(0, value);
                }

                gash::List<Tracked> destination(
                    std::move(source)
                );

                require(
                    destination.get(0).value == 11,
                    "Move lost value"
                );

                require(
                    Tracked::live == 1,
                    "Move duplicated stored object"
                );
            }

            require(
                Tracked::live == 0,
                "Moved element leaked"
            );
        }, passed);

        // Test 8: Normal C++ exception unwinding
        // automatically destroys list resources.
        run("Exception unwinding cleans up lists", [] {
            try {
                gash::List<Tracked> items(1);

                {
                    Tracked value(20);
                    items.set(0, value);
                }

                throw std::runtime_error(
                    "Simulated GASH runtime error"
                );
            }
            catch (const std::runtime_error&) {
                require(
                    Tracked::live == 0,
                    "List survived exception unwinding"
                );
            }
        }, passed);

        // Test 9: Invalid construction does not
        // leave allocated list elements behind.
        run("Invalid construction is safe", [] {
            bool threw = false;

            try {
                gash::List<Tracked> invalid(-1);
            }
            catch (const std::length_error&) {
                threw = true;
            }

            require(
                threw,
                "Negative size did not throw"
            );
        }, passed);

        std::cout
            << "\nResult: "
            << passed
            << "/9 tests passed\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
