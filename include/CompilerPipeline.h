
#ifndef COMPILERPIPELINE_H
#define COMPILERPIPELINE_H

#include <filesystem>
#include "AST.h"

namespace gash{
    class CompilerPipeline {
        public:
            void writeGeneratedCpp(const gash::Program& program,const std::filesystem::path& outputPath);
            void compileGeneratedCpp(const std::filesystem::path& cppPath,const std::filesystem::path& executablePath);
            int runExecutable(const std::filesystem::path& executablePath);
            int compileAndRun(const gash::Program& program,const std::filesystem::path& cppPath,const std::filesystem::path& executablePath);
    };
}
#endif