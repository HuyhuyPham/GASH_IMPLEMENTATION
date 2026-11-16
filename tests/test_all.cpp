// Unified entry point for the existing CTest suite and GASH example programs.
// Build: c++ -std=c++17 tests/test_all.cpp -o build/test_all
// Run:   ./build/test_all
// Optional: ./build/test_all --include-cli-test
// DO NOT register this executable in CTest: it calls CTest itself.
// Requires a POSIX shell (macOS/Linux) and g++ on PATH for `gash --run`.

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

enum class Check { Exact, GuardError, LockError, Inspect, Preview, BudgetError, Smoke };
struct TestSpec {
    Check check;
    std::string expected;
    std::string group;
};

static std::string quote(const fs::path& p) {
    std::string out = "'";
    for (char c : p.string()) out += (c == '\'') ? "'\\''" : std::string(1, c);
    return out + "'";
}

static std::string readAll(const fs::path& p) {
    std::ifstream file(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

static std::string normalize(std::string s) {
    // Allow CRLF and a final newline, but preserve internal whitespace.
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    while (!s.empty() && s.back() == '\n') s.pop_back();
    return s;
}

static bool has(const std::string& str, const std::string& part) {
    return str.find(part) != std::string::npos;
}

static bool matches(const TestSpec& spec, int runStatus, const std::string& output) {
    switch (spec.check) {
    case Check::Exact:
        return runStatus == 0 && output == spec.expected;
    case Check::GuardError:
        return runStatus != 0 && !has(output, "999") &&
            (has(output, "Guard violation") || has(output, "Guard rejected") ||
             has(output, "guard violation") || has(output, "guard rejected"));
    case Check::LockError:
        return runStatus != 0 && !has(output, "999") &&
            (has(output, "locked") || has(output, "Cannot modify") ||
             has(output, "Cannot restore locked"));
    case Check::Inspect:
        return runStatus == 0 && has(output, "List<int>") && has(output, "size=3") &&
               has(output, "[0] 7") && has(output, "[1] <uninitialized>") &&
               has(output, "[2] 9");
    case Check::Preview:
        return runStatus == 0 && has(output, "Variables (2)") &&
               has(output, "balance : int = 100 [locked]") &&
               has(output, "name : string = GASH [unlocked]");
    case Check::BudgetError:
        // BudgetScope may handle the exception and exit with status 0.
        return has(output, "budget exhausted") || has(output, "Budget exceeded") ||
               has(output, "BudgetExceeded");
    case Check::Smoke:
        // Compile + execute succeeds; this does NOT assert the exact output.
        return runStatus == 0;
    }
    return false;
}

int main(int argc, char** argv) {
    try {
        bool includeCliTest = false;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--include-cli-test") includeCliTest = true;
            else {
                std::cerr << "Unknown argument: " << argv[i] << "\n";
                return 2;
            }
        }
        const fs::path root = fs::current_path();
        const fs::path gash = root / "build" / "gash";
        const fs::path examples = root / "examples";
        if (!fs::exists(gash) || !fs::is_directory(examples)) {
            std::cerr << "Run from the repository root after building GASH:\n"
                      << "  cmake -S . -B build && cmake --build build\n";
            return 2;
        }

        // Explicit manifest: 10 advanced assertions + 3 successful checkpoints
        // + 2 expected checkpoint errors + 7 general executable smoke tests.
        const std::map<std::string, TestSpec> tests = {
            {"guard_valid",               {Check::Exact, "75", "Advanced"}},
            {"guard_violation",           {Check::GuardError, "", "Advanced"}},
            {"lock_unlock_valid",         {Check::Exact, "20", "Advanced"}},
            {"lock_violation",            {Check::LockError, "", "Advanced"}},
            {"inspect_list",              {Check::Inspect, "", "Advanced"}},
            {"preview_state",             {Check::Preview, "", "Advanced"}},
            {"budget_valid",              {Check::Exact, "42", "Advanced"}},
            {"budget_exhausted",          {Check::BudgetError, "", "Advanced"}},
            {"checkpoint_restore",        {Check::Exact, "10", "Advanced"}},
            {"combined_guard_checkpoint", {Check::Exact, "100", "Advanced"}},
            {"checkpoint_smoke",          {Check::Exact, "10", "Checkpoint success"}},
            {"checkpoint_multi",          {Check::Exact, "10\n20\n10\n20", "Checkpoint success"}},
            {"checkpoint_list",           {Check::Exact, "7\n8", "Checkpoint success"}},
            {"checkpoint_guard",          {Check::GuardError, "", "Checkpoint expected error"}},
            {"checkpoint_lock",           {Check::LockError, "", "Checkpoint expected error"}},
            {"hello",                     {Check::Smoke, "", "Basic smoke"}},
            {"variables",                 {Check::Smoke, "", "Basic smoke"}},
            {"conditions",                {Check::Smoke, "", "Basic smoke"}},
            {"loops",                     {Check::Smoke, "", "Basic smoke"}},
            {"functions",                 {Check::Smoke, "", "Basic smoke"}},
            {"lists",                     {Check::Smoke, "", "Basic smoke"}},
            {"transparency",              {Check::Smoke, "", "Basic smoke"}}
        };

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const fs::path tmp = fs::temp_directory_path() / ("gash_test_all_" + std::to_string(stamp));
        fs::create_directories(tmp);
        struct Cleanup {
            fs::path dir;
            ~Cleanup() { std::error_code ec; fs::remove_all(dir, ec); }
        } cleanup{tmp};

        std::cout << "=== Stage 1: Registered CTest tests ===\n" << std::flush;
        const int ctestStatus = std::system(
            ("cd " + quote(root) + " && ctest --test-dir build --output-on-failure").c_str());
        const bool ctestOk = ctestStatus == 0;
        std::cout << (ctestOk ? "PASS" : "FAIL") << ": CTest suite\n";

        int passed = 0, failed = 0, skipped = 0, smokePassed = 0;
        const std::string command = "cd " + quote(root) + " && " + quote(gash);
        std::cout << "\n=== Stage 2: Manifest-based GASH CLI tests ===\n";
        for (const auto& [stem, spec] : tests) {
            const fs::path src = examples / (stem + ".gash");
            std::cout << "--- [" << spec.group << "] " << stem << " ---\n" << std::flush;
            if (!fs::is_regular_file(src)) {
                std::cerr << "FAIL: missing " << src << "\n";
                ++failed;
                continue;
            }
            const fs::path input = tmp / src.filename();
            const fs::path checkLog = tmp / (stem + ".check.log");
            const fs::path runLog = tmp / (stem + ".run.log");
            fs::copy_file(src, input, fs::copy_options::overwrite_existing);

            // All manifest cases must be semantically valid. Expected failures
            // are runtime violations, NOT failures to parse or type-check.
            const int checked = std::system((command + " --check " + quote(input) +
                " < /dev/null > " + quote(checkLog) + " 2>&1").c_str());
            if (checked != 0) {
                ++failed;
                std::cerr << "FAIL: --check rejected the program:\n" << readAll(checkLog) << "\n";
                continue;
            }

            const int ran = std::system((command + " --run " + quote(input) +
                " < /dev/null > " + quote(runLog) + " 2>&1").c_str());
            const std::string output = normalize(readAll(runLog));
            if (matches(spec, ran, output)) {
                ++passed;
                if (spec.check == Check::Smoke) ++smokePassed;
                std::cout << "PASS" << (spec.check == Check::Smoke ? " (execution smoke; output not asserted)" : "") << "\n";
            } else {
                ++failed;
                std::cerr << "FAIL: unexpected result (raw status " << ran << ")\n"
                          << "Output:\n" << output << "\n";
                if (spec.check == Check::Exact) std::cerr << "Expected:\n" << spec.expected << "\n";
            }
        }

        // cli_test.gash is experimental and may require input; opt-in only.
        const fs::path cliTest = examples / "cli_test.gash";
        if (fs::exists(cliTest)) {
            if (!includeCliTest) {
                ++skipped;
                std::cout << "SKIP: cli_test.gash (experimental; pass --include-cli-test to smoke-test)\n";
            } else {
                const fs::path input = tmp / "cli_test.gash";
                const fs::path checkLog = tmp / "cli_test.check.log";
                const fs::path runLog = tmp / "cli_test.run.log";
                fs::copy_file(cliTest, input, fs::copy_options::overwrite_existing);
                const int checked = std::system((command + " --check " + quote(input) +
                    " < /dev/null > " + quote(checkLog) + " 2>&1").c_str());
                const int ran = checked == 0 ? std::system((command + " --run " + quote(input) +
                    " < /dev/null > " + quote(runLog) + " 2>&1").c_str()) : -1;
                if (checked == 0 && ran == 0) {
                    ++passed;
                    ++smokePassed;
                    std::cout << "PASS: cli_test.gash (opt-in smoke test)\n";
                } else {
                    ++failed;
                    std::cerr << "FAIL: cli_test.gash\n"
                              << (checked != 0 ? readAll(checkLog) : readAll(runLog)) << "\n";
                }
            }
        }

        // Don't silently claim that newly added examples were tested.
        for (const auto& file : fs::directory_iterator(examples)) {
            if (!file.is_regular_file() || file.path().extension() != ".gash") continue;
            const std::string stem = file.path().stem().string();
            if (tests.count(stem) == 0 && stem != "cli_test") {
                ++skipped;
                std::cout << "SKIP: " << file.path().filename().string()
                          << " (not in manifest; add an expected outcome to test_all.cpp)\n";
            }
        }

        // Stage 3: Compile-time rejection tests. Unlike the runtime-error
        // examples in Stage 2, every source below MUST fail `gash --check`.
        // Do not call --run for these intentionally invalid programs.
        const fs::path negatives = root / "tests" / "negative";
        const std::vector<std::string> negativeFiles = {
            "undefined_variable.gash",
            "assign_type_mismatch.gash",
            "duplicate_declaration.gash",
            "missing_semicolon.gash",
            "wrong_argument_count.gash",
            "invalid_expression.gash"
        };
        int negativePassed = 0;
        int negativeFailed = 0;
        std::cout << "\n=== Stage 3: Negative compile-time tests ===\n";
        for (const std::string& filename : negativeFiles) {
            const fs::path src = negatives / filename;
            const fs::path log = tmp / (filename + ".negative.log");
            std::cout << "--- " << filename << " ---\n" << std::flush;
            if (!fs::is_regular_file(src)) {
                ++negativeFailed;
                std::cerr << "FAIL: missing " << src << "\n";
                continue;
            }
            const int status = std::system((command + " --check " + quote(src) +
                " < /dev/null > " + quote(log) + " 2>&1").c_str());
            const std::string diagnostics = normalize(readAll(log));
            // Rejection must be genuine: fail if the compiler accidentally
            // accepts the program or emits no meaningful diagnostic.
            if (status != 0 && !diagnostics.empty()) {
                ++negativePassed;
                std::cout << "PASS: rejected during --check\n";
            } else {
                ++negativeFailed;
                std::cerr << "FAIL: expected nonzero --check and diagnostic"
                          << " (status " << status << ")\n"
                          << "Output:\n" << diagnostics << "\n";
            }
        }

        std::cout << "\n=== Summary ===\n"
                  << "CTest: " << (ctestOk ? "PASS" : "FAIL") << "\n"
                  << "GASH CLI: " << passed << " passed, " << failed << " failed, "
                  << skipped << " skipped (" << smokePassed << " unasserted smoke passes)\n"
                  << "Negative compilation: " << negativePassed << " passed, "
                  << negativeFailed << " failed\n";
        return (ctestOk && failed == 0 && negativeFailed == 0) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Runner error: " << e.what() << "\n";
        return 2;
    }
}
