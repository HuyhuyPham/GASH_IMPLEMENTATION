// Targeted end-to-end compiler regression tests for Transparent GASH.
// POSIX (macOS/Linux), C++17. Does not modify the source repository.
// Usage: test_advanced_pipeline <path-to-gash> [repository-root]
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <cerrno>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

enum class Expect { ExactOutput, ListOutput, RuntimeError, RuntimeErrorWithOutput, SyntaxLocation };
struct Case { std::string name, source; Expect expect; std::string expected; };
struct Result { int code = -1; bool timedOut = false; std::string out, err; };

static std::string readText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}
static std::string normalized(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    while (!text.empty() && text.back() == '\n') text.pop_back();
    return text;
}
static std::string clip(std::string s, std::size_t limit=350) {
    if (s.size() > limit) { s.resize(limit); s += " ..."; }
    return s;
}

// fork/exec avoids shell quoting problems for repository paths containing spaces.
static Result runProcess(const fs::path& executable, const std::vector<std::string>& args,
                         const fs::path& cwd, const fs::path& logPrefix,
                         std::chrono::seconds timeout=45s) {
    const fs::path outPath = logPrefix.string() + ".stdout";
    const fs::path errPath = logPrefix.string() + ".stderr";
    pid_t pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");
    if (pid == 0) {
        setpgid(0, 0); // isolate descendant C++ compilation for timeout cleanup
        if (chdir(cwd.c_str()) != 0) _exit(126);
        int input = open("/dev/null", O_RDONLY);
        int out = open(outPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        int err = open(errPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (input < 0 || out < 0 || err < 0) _exit(126);
        if (dup2(input, STDIN_FILENO) < 0 || dup2(out, STDOUT_FILENO) < 0 ||
            dup2(err, STDERR_FILENO) < 0) _exit(126);
        close(input); close(out); close(err);
        std::vector<std::string> argvStrings = {executable.string()};
        argvStrings.insert(argvStrings.end(), args.begin(), args.end());
        std::vector<char*> argv;
        for (auto& s : argvStrings) argv.push_back(s.data());
        argv.push_back(nullptr);
        execv(executable.c_str(), argv.data());
        _exit(127);
    }
    int status=0; bool timedOut=false;
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        const pid_t done = waitpid(pid, &status, WNOHANG);
        if (done == pid) break;
        if (done < 0) {
            if (errno == EINTR) continue;
            kill(-pid, SIGKILL); waitpid(pid, &status, 0);
            throw std::runtime_error("waitpid failed");
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            timedOut=true; kill(-pid, SIGKILL); waitpid(pid, &status, 0); break;
        }
        std::this_thread::sleep_for(50ms);
    }
    Result result;
    result.timedOut = timedOut;
    result.code = !timedOut && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    result.out = readText(outPath); result.err = readText(errPath);
    return result;
}

int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 3) {
            std::cerr << "Usage: test_advanced_pipeline <path-to-gash> [repository-root]\n";
            return 2;
        }
        const fs::path gash = fs::absolute(argv[1]);
        const fs::path root = argc == 3 ? fs::absolute(argv[2]) : fs::current_path();
        if (!fs::is_regular_file(gash)) {
            std::cerr << "Cannot find GASH executable: " << gash << '\n';
            return 2;
        }
        auto scratch = (fs::temp_directory_path() / "gash_advanced_pipeline_XXXXXX").string();
        std::vector<char> tempName(scratch.begin(), scratch.end());
        tempName.push_back('\0');
        char* temp = mkdtemp(tempName.data());
        if (!temp) throw std::runtime_error("Could not create temporary directory");
        const fs::path dir(temp);
        struct Cleanup { fs::path p; ~Cleanup() { std::error_code ec; fs::remove_all(p,ec); } } cleanup{dir};

        const std::vector<Case> cases = {
            {"global_in_function", "var g = 10;\nfunc addG(int x) { return x + g; }\noutput(addG(5));\n", Expect::ExactOutput, "15"},
            {"output_bool", "var flag = true;\noutput(flag);\n", Expect::ExactOutput, "true"},
            {"output_list", "var nums = list<int>(2);\nnums[0] = 7;\nnums[1] = 8;\noutput(nums);\n", Expect::ListOutput, ""},
            {"divide_zero", "var x = 10;\noutput(x / 0);\n", Expect::RuntimeError, "Division by zero"},
            {"modulo_zero", "var x = 10;\noutput(x % 0);\n", Expect::RuntimeError, "Modulo by zero"},
            {"power_zero_negative", "output(0 ^ -1);\n", Expect::RuntimeError, "Invalid exponentiation"},
            {"repeat_negative", "repeat(-3) { output(111); }\noutput(222);\n", Expect::RuntimeError, "Repeat count cannot be negative"},
            {"char_quote", "output('\\'');\n", Expect::ExactOutput, "'"},
            {"char_nul", "var x = '\\0';\noutput(int(x));\n", Expect::ExactOutput, "0"},
            {"nested_function", "var base = 2;\n{\n  var offset = 3;\n  func helper(int x) { return x + offset; }\n  output(helper(base));\n}\n", Expect::ExactOutput, "5"},
            {"list_parameter", "func first(list<int> xs) { return xs[0]; }\nvar xs = list<int>(1);\nxs[0] = 9;\noutput(first(xs));\n", Expect::ExactOutput, "9"},
            {"missing_return", "func maybe(bool condition) {\n  if (condition) { return 7; }\n}\noutput(maybe(false));\n", Expect::RuntimeError, "reached end without returning"},
            {"caught_runtime_error", "var balance = 10;\nguard balance >= 0;\noutput(123);\nbalance = -1;\n", Expect::RuntimeErrorWithOutput, "123"},
            {"syntax_location", "var x = 1;\nvar y = 2\noutput(x);\n", Expect::SyntaxLocation, ""}
        };

        std::cout << "=== Advanced compilation and runtime regression tests ===\n";
        int passed = 0;
        for (const auto& test : cases) {
            const fs::path source = dir / (test.name + ".gash");
            { std::ofstream file(source); file << test.source; }
            const Result check = runProcess(gash, {"--check", source.string()}, root, dir / (test.name + ".check"));
            bool ok = false;
            Result exec;
            if (test.expect == Expect::SyntaxLocation) {
                const std::regex location(R"((line\s*[0-9]+.{0,80}col(?:umn)?\s*[0-9]+)|([0-9]+\s*:\s*[0-9]+))", std::regex::icase);
                const std::string diagnostic = check.err + "\n" + check.out;
                ok = !check.timedOut && check.code != 0 && std::regex_search(diagnostic,location);
            } else if (!check.timedOut && check.code == 0) {
                const fs::path cwd = test.name == "run_outside_repo" ? dir : root;
                exec = runProcess(gash, {"--run", source.string()}, cwd, dir / (test.name + ".run"));
                const std::string out = normalized(exec.out);
                const std::string err = normalized(exec.err);
                if (!exec.timedOut) {
                    switch (test.expect) {
                    case Expect::ExactOutput: ok = exec.code == 0 && out == test.expected; break;
                    case Expect::ListOutput:
                        ok = exec.code == 0 && out.find("List<int>") != std::string::npos &&
                             out.find("size=2") != std::string::npos && out.find("[0] 7") != std::string::npos &&
                             out.find("[1] 8") != std::string::npos; break;
                    case Expect::RuntimeError:
                        ok = exec.code != 0 && err.find("Runtime error:") != std::string::npos &&
                             err.find(test.expected) != std::string::npos; break;
                    case Expect::RuntimeErrorWithOutput:
                        ok = exec.code != 0 && out == test.expected &&
                             err.find("Runtime error: Guard violation") != std::string::npos; break;
                    case Expect::SyntaxLocation: break;
                    }
                }
            }
            std::cout << (ok ? "PASS" : "FAIL") << ": " << test.name << '\n';
            if (ok) ++passed;
            else std::cerr << "  check: " << check.code << " " << clip(check.err) << "\n"
                           << "  run: " << exec.code << " stdout=[" << clip(exec.out)
                           << "] stderr=[" << clip(exec.err) << "]\n";
        }
        // In addition to the 14 cases above, ensure --run resolves runtime assets
        // when invoked with a working directory outside the repository.
        const fs::path outside = dir / "run_outside_repo.gash";
        { std::ofstream f(outside); f << "output(42);\n"; }
        const Result outsideCheck = runProcess(gash, {"--check", outside.string()}, root, dir / "outside.check");
        const Result outsideRun = runProcess(gash, {"--run", outside.string()}, dir, dir / "outside.run");
        const bool outsideOk = outsideCheck.code == 0 && outsideRun.code == 0 && !outsideRun.timedOut &&
                               normalized(outsideRun.out) == "42";
        std::cout << (outsideOk ? "PASS" : "FAIL") << ": run_outside_repo\n";
        if (!outsideOk) std::cerr << "  " << clip(outsideRun.err) << '\n';
        passed += outsideOk ? 1 : 0;
        std::cout << "\nAdvanced regression summary: " << passed << "/15 passed\n";
        return passed == 15 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Advanced test runner error: " << e.what() << '\n';
        return 2;
    }
}
