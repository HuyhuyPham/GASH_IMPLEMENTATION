cmake_minimum_required(VERSION 3.16)

project(TransparentGASH
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Shared compiler and runtime components
add_library(gash_core
    src/Lexer.cpp
    src/Parser.cpp
    src/Semantic.cpp
    src/CodeGenerator.cpp
    src/CompilerPipeline.cpp
    runtime/Budget.cpp
    runtime/Checkpoint.cpp
    runtime/Guard.cpp
    runtime/Environment.cpp
    runtime/Inspect.cpp
    runtime/Preview.cpp
)

target_include_directories(gash_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)

# Main CLI
add_executable(gash
    src/main.cpp
)

target_link_libraries(gash
    PRIVATE gash_core
)

# Enable automated testing
include(CTest)

if(BUILD_TESTING)
    add_executable(test_lexer tests/test_lexer.cpp)
    target_link_libraries(test_lexer PRIVATE gash_core)
    add_test(NAME LexerTests COMMAND test_lexer)

    add_executable(test_ast tests/test_ast.cpp)
    target_link_libraries(test_ast PRIVATE gash_core)
    add_test(NAME AstTests COMMAND test_ast)

    add_executable(test_parser tests/test_parser.cpp)
    target_link_libraries(test_parser PRIVATE gash_core)
    add_test(NAME ParserTests COMMAND test_parser)

    add_executable(test_statement_parser tests/test_statement_parser.cpp)
    target_link_libraries(test_statement_parser PRIVATE gash_core)
    add_test(NAME StatementParserTests COMMAND test_statement_parser)

    add_executable(test_lexer_parser tests/test_lexer_parser.cpp)
    target_link_libraries(test_lexer_parser PRIVATE gash_core)
    add_test(NAME LexerParserTests COMMAND test_lexer_parser)

    add_executable(test_parser_errors tests/test_parser_errors.cpp)
    target_link_libraries(test_parser_errors PRIVATE gash_core)
    add_test(NAME ParserErrorTests COMMAND test_parser_errors)

    add_executable(test_parser_nested tests/test_parser_nested.cpp)
    target_link_libraries(test_parser_nested PRIVATE gash_core)
    add_test(NAME ParserNestedTests COMMAND test_parser_nested)

    add_executable(test_semantic tests/test_semantic.cpp)
    target_link_libraries(test_semantic PRIVATE gash_core)
    add_test(NAME SemanticTests COMMAND test_semantic)

    add_executable(test_guards tests/test_guards.cpp)
    target_link_libraries(test_guards PRIVATE gash_core)
    add_test(NAME GuardLockEnvironmentTests COMMAND test_guards)

    add_executable(test_checkpoint tests/test_checkpoint.cpp)
    target_link_libraries(test_checkpoint PRIVATE gash_core)
    add_test(NAME CheckpointTests COMMAND test_checkpoint)

    add_executable(test_checkpoint_runtime tests/test_checkpoint_runtime.cpp)
    target_link_libraries(test_checkpoint_runtime PRIVATE gash_core)
    add_test(NAME CheckpointRuntimeTests COMMAND test_checkpoint_runtime)

    add_executable(test_codegen_checkpoint tests/test_codegen_checkpoint.cpp)
    target_link_libraries(test_codegen_checkpoint PRIVATE gash_core)
    add_test(NAME CheckpointCodegenTests COMMAND test_codegen_checkpoint)

    add_executable(test_list tests/test_list.cpp)
    target_link_libraries(test_list PRIVATE gash_core)
    add_test(NAME ListTests COMMAND test_list)

    add_executable(test_inspect_preview tests/test_inspect_preview.cpp)
    target_link_libraries(test_inspect_preview PRIVATE gash_core)
    add_test(NAME InspectPreviewTests COMMAND test_inspect_preview)

    add_executable(test_auto_cleanup tests/test_auto_cleanup.cpp)
    target_link_libraries(test_auto_cleanup PRIVATE gash_core)
    add_test(NAME AutoCleanupTests COMMAND test_auto_cleanup)

    add_executable(test_codegen tests/test_codegen.cpp)
    target_link_libraries(test_codegen PRIVATE gash_core)
    add_test(NAME CodegenTests COMMAND test_codegen)

    add_executable(test_s2_codegen tests/test_s2_codegen.cpp)
    target_link_libraries(test_s2_codegen PRIVATE gash_core)
    add_test(NAME S2CodegenTests COMMAND test_s2_codegen)

    add_executable(test_s3_codegen tests/test_s3_codegen.cpp)
    target_link_libraries(test_s3_codegen PRIVATE gash_core)
    add_test(NAME S3CodegenTests COMMAND test_s3_codegen)

    add_executable(test_integration tests/test_integration.cpp)
    target_link_libraries(test_integration PRIVATE gash_core)
    add_test(NAME IntegrationTests COMMAND test_integration)
endif()
