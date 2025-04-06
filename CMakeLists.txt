cmake_minimum_required(VERSION 3.16)
project(trading_engine VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/order_book.cpp
    src/matching_engine.cpp
)

# Library for shared code
add_library(trading_core STATIC
    src/order_book.cpp
    src/matching_engine.cpp
)
target_include_directories(trading_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Create the main executable
add_executable(trading_engine ${SOURCES})
target_link_libraries(trading_engine PRIVATE trading_core)

# Include directories
target_include_directories(trading_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enable testing
set(BUILD_TESTING ON)
enable_testing()

# Basic tests
add_executable(trading_tests 
    src/tests.cpp 
)
target_link_libraries(trading_tests PRIVATE trading_core)
target_include_directories(trading_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME BasicTests COMMAND trading_tests)

# Advanced tests
add_executable(advanced_tests 
    src/advanced_tests.cpp 
)
target_link_libraries(advanced_tests PRIVATE trading_core)
target_include_directories(advanced_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME AdvancedTests COMMAND advanced_tests)

# Benchmarks
add_executable(trading_benchmarks 
    src/benchmarks.cpp 
)
target_link_libraries(trading_benchmarks PRIVATE trading_core)
target_include_directories(trading_benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
