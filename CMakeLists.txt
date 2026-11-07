
cmake_minimum_required(VERSION 3.16)

project(TransparentGASH
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Shared compiler components
add_library(gash_core
    src/Lexer.cpp
    src/Parser.cpp
    src/CodeGenerator.cpp
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

    add_executable(test_parser tests/test_parser.cpp)
    target_link_libraries(test_parser PRIVATE gash_core)
    add_test(NAME ParserTests COMMAND test_parser)

    add_executable(test_list tests/test_list.cpp)
    target_link_libraries(test_list PRIVATE gash_core)
    add_test(NAME ListTests COMMAND test_list)

endif()