#include "CompilerPipeline.h"
#include "CodeGenerator.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace gash
{
    namespace
    {
        int runProcess(const std::string &program, const std::vector<std::string> &args)
        {
#ifdef _WIN32
            // Build quoted command line for Windows _spawnvp / system
            std::vector<const char *> argv;
            std::vector<std::string> quotedArgs;
            quotedArgs.reserve(args.size());
            for (const auto &arg : args)
            {
                if (arg.find_first_of(" \t\"") != std::string::npos)
                {
                    std::string q = "\"";
                    for (char c : arg)
                    {
                        if (c == '"')
                        {
                            q += "\\\"";
                        }
                        else
                        {
                            q += c;
                        }
                    }
                    q += "\"";
                    quotedArgs.push_back(std::move(q));
                }
                else
                {
                    quotedArgs.push_back(arg);
                }
            }
            for (const auto &q : quotedArgs)
            {
                argv.push_back(q.c_str());
            }
            argv.push_back(nullptr);

            intptr_t status = _spawnvp(_P_WAIT, program.c_str(), argv.data());
            if (status == -1)
            {
                throw std::runtime_error(
                    "Failed to execute process '" + program + "': " +
                    std::string(std::strerror(errno)));
            }
            return static_cast<int>(status);
#else
            std::vector<std::string> mutableArgs = args;
            std::vector<char *> argv;
            argv.reserve(mutableArgs.size() + 1);
            for (auto &arg : mutableArgs)
            {
                argv.push_back(arg.data());
            }
            argv.push_back(nullptr);

            pid_t pid;
            int result = posix_spawn(
                &pid,
                program.c_str(),
                nullptr,
                nullptr,
                argv.data(),
                environ);
            if (result != 0)
            {
                result = posix_spawnp(
                    &pid,
                    program.c_str(),
                    nullptr,
                    nullptr,
                    argv.data(),
                    environ);
            }
            if (result != 0)
            {
                throw std::runtime_error(
                    "Failed to execute process '" + program + "': " +
                    std::string(std::strerror(result)));
            }

            int status = 0;
            pid_t waited;
            do
            {
                waited = waitpid(pid, &status, 0);
            } while (waited == -1 && errno == EINTR);

            if (waited == -1)
            {
                throw std::runtime_error("Failed to wait for process '" + program + "'");
            }
            if (WIFSIGNALED(status))
            {
                throw std::runtime_error(
                    "Program terminated by signal: " +
                    std::to_string(WTERMSIG(status)));
            }
            if (!WIFEXITED(status))
            {
                throw std::runtime_error("Program did not exit normally");
            }
            return WEXITSTATUS(status);
#endif
        }
    } // anonymous namespace

    void CompilerPipeline::writeGeneratedCpp(
        const Program &program,
        const std::filesystem::path &outputPath)
    {
        CodeGenerator generator;
        std::string cppCode = generator.generate(program);

        if (!outputPath.parent_path().empty())
        {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open())
        {
            throw std::runtime_error(
                "Cannot create output file: " + outputPath.string());
        }

        outputFile << cppCode;
        outputFile.close();

        if (!outputFile)
        {
            throw std::runtime_error(
                "Failed to write generated C++ file: " + outputPath.string());
        }
    }

    void CompilerPipeline::compileGeneratedCpp(
        const std::filesystem::path &cppPath,
        const std::filesystem::path &executablePath)
    {
        if (!std::filesystem::is_regular_file(cppPath))
        {
            throw std::runtime_error(
                "Generated C++ file does not exist: " + cppPath.string());
        }

        if (!executablePath.parent_path().empty())
        {
            std::filesystem::create_directories(executablePath.parent_path());
        }

        std::filesystem::path repoRoot =
            std::filesystem::path(__FILE__).parent_path().parent_path();
        std::filesystem::path includeDir = repoRoot / "include";
        std::filesystem::path runtimeDir = repoRoot / "runtime";

        std::vector<std::string> args = {
            "g++",
            "-std=c++17",
            "-I" + includeDir.string(),
            "-I" + runtimeDir.string(),
            cppPath.string(),
            (runtimeDir / "Budget.cpp").string(),
            (runtimeDir / "Checkpoint.cpp").string(),
            (runtimeDir / "Guard.cpp").string(),
            (runtimeDir / "Environment.cpp").string(),
            (runtimeDir / "Inspect.cpp").string(),
            (runtimeDir / "Preview.cpp").string(),
            "-o",
            executablePath.string()};

        int exitCode = runProcess("g++", args);
        if (exitCode != 0)
        {
            throw std::runtime_error(
                "C++ compilation failed with exit code " +
                std::to_string(exitCode));
        }
    }

    int CompilerPipeline::runExecutable(
        const std::filesystem::path &executablePath)
    {
        if (!std::filesystem::is_regular_file(executablePath))
        {
            throw std::runtime_error(
                "Executable does not exist: " + executablePath.string());
        }

        std::string path = std::filesystem::absolute(executablePath).string();
        return runProcess(path, {path});
    }

    int CompilerPipeline::compileAndRun(
        const gash::Program &program,
        const std::filesystem::path &cppPath,
        const std::filesystem::path &executablePath)
    {
        try
        {
            writeGeneratedCpp(program, cppPath);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "Code generation failed: " + std::string(e.what()));
        }

        try
        {
            compileGeneratedCpp(cppPath, executablePath);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "C++ compilation failed: " + std::string(e.what()));
        }

        try
        {
            return runExecutable(executablePath);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "Program execution failed: " + std::string(e.what()));
        }
    }
}
