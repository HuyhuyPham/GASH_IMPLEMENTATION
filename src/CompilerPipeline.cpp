
#include "CompilerPipeline.h"
#include "CodeGenerator.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <spawn.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <vector>

extern char **environ;

namespace gash
{
    void CompilerPipeline::writeGeneratedCpp(
        const Program &program,
        const std::filesystem::path &outputPath)
    {
        // Generate C++ source code from the AST.
        CodeGenerator generator;
        std::string cppCode = generator.generate(program);

        // Create the output directory if necessary.
        if (!outputPath.parent_path().empty())
        {
            std::filesystem::create_directories(
                outputPath.parent_path());
        }

        // Open the generated C++ file.
        std::ofstream outputFile(outputPath);

        if (!outputFile.is_open())
        {
            throw std::runtime_error(
                "Cannot create output file: " +
                outputPath.string());
        }

        // Write generated C++ code to the file.
        outputFile << cppCode;
        outputFile.close();

        if (!outputFile)
        {
            throw std::runtime_error(
                "Failed to write generated C++ file: " +
                outputPath.string());
        }
    }

    int CompilerPipeline::compileAndRun(
        const gash::Program &program,
        const std::filesystem::path &cppPath,
        const std::filesystem::path &executablePath)
    {
        // Stage 1: Generate C++
        try
        {
            writeGeneratedCpp(program, cppPath);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "Code generation failed: " +
                std::string(e.what()));
        }

        // Stage 2: Compile generated C++
        try
        {
            compileGeneratedCpp(cppPath, executablePath);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "C++ compilation failed: " +
                std::string(e.what()));
        }

        // Stage 3: Execute the program
        try
        {
            return runExecutable(executablePath);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(
                "Program execution failed: " +
                std::string(e.what()));
        }
    }

    int CompilerPipeline::runExecutable(
        const std::filesystem::path &executablePath)
    {
        // Check whether the executable exists.
        if (!std::filesystem::is_regular_file(executablePath))
        {
            throw std::runtime_error(
                "Executable does not exist: " +
                executablePath.string());
        }

        // Use an absolute path to locate the executable.
        std::string path =
            std::filesystem::absolute(executablePath).string();

        char *argv[] = {
            path.data(),
            nullptr};

        pid_t pid;

        // Start the executable.
        int result = posix_spawn(
            &pid,
            path.c_str(),
            nullptr,
            nullptr,
            argv,
            environ);

        if (result != 0)
        {
            throw std::runtime_error(
                "Failed to execute program: " +
                std::string(std::strerror(result)));
        }

        // Wait for the program to finish.
        int status = 0;
        pid_t waited;

        do
        {
            waited = waitpid(pid, &status, 0);
        } while (waited == -1 && errno == EINTR);

        if (waited == -1)
        {
            throw std::runtime_error(
                "Failed to wait for executable");
        }

        if (WIFSIGNALED(status))
        {
            throw std::runtime_error(
                "Program terminated by signal: " +
                std::to_string(WTERMSIG(status)));
        }

        if (!WIFEXITED(status))
        {
            throw std::runtime_error(
                "Program did not exit normally");
        }

        return WEXITSTATUS(status);
    }

    int CompilerPipeline::compileAndRun(
        const gash::Program &program,
        const std::filesystem::path &cppPath,
        const std::filesystem::path &executablePath)
    {
        // Step 1: Generate C++ source code
        writeGeneratedCpp(program, cppPath);

        // Step 2: Compile the generated C++ code
        compileGeneratedCpp(cppPath, executablePath);

        // Step 3: Execute the compiled program
        int exitCode = runExecutable(executablePath);

        // Return the program's exit status
        return exitCode;
    }
}