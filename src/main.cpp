#include "CodeGenerator.h"
#include "CompilerPipeline.h"
#include "Lexer.h"
#include "LexicalError.h"
#include "Parser.h"
#include "Semantic.h"
#include "Token.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

void printUsage() {
    std::cout
        << "Transparent GASH Compiler\n\n"
        << "Usage:\n"
        << "  gash --help\n"
        << "  gash --tokens <file.gash>\n"
        << "  gash --check <file.gash>\n"
        << "  gash --emit-cpp <file.gash> [output.cpp]\n"
        << "  gash --run <file.gash>\n"
        << "  gash <file.gash>\n";
}

std::string readSourceFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error(
            "Cannot open source file: " + path.string()
        );
    }

    std::string source(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    );

    if (file.bad()) {
        throw std::runtime_error(
            "Failed to read source file: " + path.string()
        );
    }

    return source;
}

void writeFile(
    const fs::path& outputPath,
    const std::string& content
) {
    if (!outputPath.parent_path().empty()) {
        fs::create_directories(outputPath.parent_path());
    }

    std::ofstream file(outputPath, std::ios::binary);

    if (!file) {
        throw std::runtime_error(
            "Cannot create output file: " + outputPath.string()
        );
    }

    file << content;
    file.close();

    if (!file) {
        throw std::runtime_error(
            "Failed to write output file: " + outputPath.string()
        );
    }
}

std::vector<gash::Token> tokenize(
    const std::string& source
) {
    gash::Lexer lexer(source);
    return lexer.tokenize();
}

gash::Program parse(
    std::vector<gash::Token> tokens
) {
    gash::Parser parser(std::move(tokens));
    return parser.parseProgram();
}

void printTokens(const std::vector<gash::Token>& tokens) {
    for (const auto& token : tokens) {
        std::cout
            << gash::tokenTypeName(token.type)
            << " | " << token.lexeme
            << " | line " << token.line
            << ", column " << token.column
            << '\n';
    }
}

fs::path defaultOutputPath(const fs::path& inputPath) {
    fs::path output = inputPath;
    output.replace_extension(".cpp");
    return output;
}

fs::path defaultExecutablePath(const fs::path& inputPath) {
    fs::path output = inputPath;
#ifdef _WIN32
    output.replace_extension(".exe");
#else
    output.replace_extension(".out");
#endif
    return output;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        printUsage();
        return 0;
    }

    if (argc < 2) {
        printUsage();
        return 1;
    }

    enum class Mode {
        Tokens,
        Check,
        EmitCpp,
        Run
    };

    Mode mode = Mode::EmitCpp;
    fs::path inputPath;
    fs::path outputPath;

    const std::string firstArg = argv[1];

    if (firstArg == "--tokens" && argc == 3) {
        mode = Mode::Tokens;
        inputPath = argv[2];

    } else if (firstArg == "--check" && argc == 3) {
        mode = Mode::Check;
        inputPath = argv[2];

    } else if (firstArg == "--run" && argc == 3) {
        mode = Mode::Run;
        inputPath = argv[2];

    } else if (firstArg == "--emit-cpp" &&
               (argc == 3 || argc == 4)) {
        mode = Mode::EmitCpp;
        inputPath = argv[2];

        if (argc == 4) {
            outputPath = argv[3];
        }

    } else if (argc == 2 && firstArg[0] != '-') {
        inputPath = firstArg;

    } else {
        std::cerr << "Error: Invalid arguments.\n\n";
        printUsage();
        return 1;
    }

    try {
        const std::string source = readSourceFile(inputPath);

        auto tokens = tokenize(source);

        if (mode == Mode::Tokens) {
            printTokens(tokens);
            return 0;
        }

        gash::Program program = parse(std::move(tokens));

        gash::SemanticAnalyzer semantic;
        semantic.analyze(program);

        if (mode == Mode::Check) {
            std::cout
                << "Lexical, syntax, and semantic checks passed.\n";
            return 0;
        }

        if (mode == Mode::Run) {
            gash::CompilerPipeline pipeline;
            fs::path cppPath = defaultOutputPath(inputPath);
            fs::path exePath = defaultExecutablePath(inputPath);
            return pipeline.compileAndRun(program, cppPath, exePath);
        }

        gash::CodeGenerator generator;
        std::string cppCode = generator.generate(program);

        if (outputPath.empty()) {
            outputPath = defaultOutputPath(inputPath);
        }

        writeFile(outputPath, cppCode);

        std::cout
            << "Generated C++: " << outputPath.string()
            << '\n';

        return 0;

    } catch (const gash::LexicalError& error) {
        std::cerr
            << inputPath.string()
            << ":" << error.line()
            << ":" << error.column()
            << ": lexical error: "
            << error.what()
            << '\n';

        return 1;

    } catch (const std::exception& error) {
        std::cerr
            << "GASH error: " << error.what()
            << '\n';

        return 1;
    }
}
