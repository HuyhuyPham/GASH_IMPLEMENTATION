#include "Inspect.h"
#include "Preview.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int passed = 0;
int total = 0;

void test(const char* name, void (*fn)()) {
    ++total;
    try {
        fn();
        ++passed;
        std::cout << "[PASS] " << name << '\n';
    } catch (const std::exception& e) {
        std::cout << "[FAIL] " << name << ": " << e.what() << '\n';
    }
}

void expect(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

void emptyList() {
    gash::List<int> list(0);
    std::ostringstream out;
    gash::inspect(list, out);
    expect(out.str().find("List<int> size=0") != std::string::npos, "wrong empty metadata");
}
void listMetadata() {
    gash::List<int> list(3);
    list.set(0, 10);
    std::ostringstream out;
    gash::inspect(list, out);
    expect(out.str().find("size=3 capacity=") != std::string::npos, "wrong metadata");
    expect(out.str().find("[0] 10") != std::string::npos, "missing initialized value");
    expect(out.str().find("[1] <uninitialized>") != std::string::npos, "missing uninitialized status");
}
void readOnly() {
    gash::List<int> list(2);
    list.set(0, 3);
    auto beforeSize = list.size();
    auto beforeCapacity = list.capacity();
    std::ostringstream out;
    gash::inspect(list, out);
    expect(list.size() == beforeSize && list.capacity() == beforeCapacity, "metadata changed");
    expect(list.get(0) == 3 && !list.isInitialized(1), "list changed");
}
void stringAndBool() {
    gash::List<std::string> strings(1);
    strings.set(0, "hello world");
    std::ostringstream sout;
    gash::inspect(strings, sout);
    expect(sout.str().find("List<string>") != std::string::npos, "wrong string type");
    expect(sout.str().find("\"hello world\"") != std::string::npos, "string not quoted");
    gash::List<bool> flags(1);
    flags.set(0, true);
    std::ostringstream bout;
    gash::inspect(flags, bout);
    expect(bout.str().find("[0] true") != std::string::npos, "wrong bool display");
}
void resizeOutput() {
    gash::List<int> list(3);
    list.set(2, 9);
    list.resize(2);
    list.resize(3);
    std::ostringstream out;
    gash::inspect(list, out);
    expect(out.str().find("[2] <uninitialized>") != std::string::npos, "stale value visible");
}
void emptyPreview() {
    std::ostringstream out;
    gash::previewSnapshot({}, out);
    expect(out.str() == "Variables (0):\n", "empty preview incorrect");
}
void snapshotFormatting() {
    std::vector<gash::VariableSnapshot> vars = {
        {"x", "int", "12", false, "global"},
        {"items", "list<int>", "size=2", true, "block:1"}
    };
    std::ostringstream out;
    gash::previewSnapshot(vars, out);
    expect(out.str().find("Variables (2):") != std::string::npos, "wrong count");
    expect(out.str().find("x : int = 12 [unlocked] scope=global") != std::string::npos, "wrong unlocked state");
    expect(out.str().find("items : list<int> = size=2 [locked] scope=block:1") != std::string::npos, "wrong locked state");
}
void absentProviderFails() {
    gash::setPreviewProvider({});
    std::ostringstream out;
    bool threw = false;
    try { gash::preview(out); } catch (const std::logic_error&) { threw = true; }
    expect(threw, "preview should not silently show empty state");
}
void providerSnapshot() {
    int count = 0;
    gash::setPreviewProvider([&count]() {
        ++count;
        return std::vector<gash::VariableSnapshot>{{"n", "int", std::to_string(count), false, "global"}};
    });
    std::ostringstream one;
    gash::preview(one);
    std::ostringstream two;
    gash::preview(two);
    expect(one.str().find("n : int = 1") != std::string::npos, "first snapshot wrong");
    expect(two.str().find("n : int = 2") != std::string::npos, "not a live snapshot");
    gash::setPreviewProvider({});
}
}

int main() {
    test("Empty list", emptyList);
    test("List metadata and occupancy", listMetadata);
    test("Inspect is read-only", readOnly);
    test("String and boolean formatting", stringAndBool);
    test("Resize does not expose old values", resizeOutput);
    test("Empty variable snapshot", emptyPreview);
    test("Snapshot formatting and lock status", snapshotFormatting);
    test("Missing environment provider is explicit", absentProviderFails);
    test("Provider returns live snapshot", providerSnapshot);
    std::cout << "\nResult: " << passed << '/' << total << " tests passed\n";
    return passed == total ? 0 : 1;
}
