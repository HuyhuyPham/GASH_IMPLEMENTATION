
// Declares CompilerPipeline, which orchestrates C++ code emission, g++ compilation, and binary execution.
#ifndef COMPILERPIPELINE_H
#define COMPILERPIPELINE_H

#include <filesystem>
#include "AST.h"

namespace gash {
    // Coordinates C++ code emission, invocation of the system C++17 compiler (g++),
    // and execution of the resulting native binary for GASH programs.
    class CompilerPipeline {
        public:
            // Generates C++17 code from a GASH AST and writes it to outputPath.
            void writeGeneratedCpp(const gash::Program& program, const std::filesystem::path& outputPath);

            // Compiles generated C++ source and GASH runtime sources into executablePath.
            void compileGeneratedCpp(const std::filesystem::path& cppPath, const std::filesystem::path& executablePath);

            // Executes the compiled program and returns its process exit code.
            int runExecutable(const std::filesystem::path& executablePath);

            // Runs code generation, compilation, and execution end-to-end.
            int compileAndRun(const gash::Program& program, const std::filesystem::path& cppPath, const std::filesystem::path& executablePath);
    };
}
#endif