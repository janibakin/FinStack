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
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
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

# Create the main executable
add_executable(trading_engine ${SOURCES})

# Include directories
target_include_directories(trading_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enable testing but only build if explicitly requested
option(BUILD_TESTING "Build the testing targets" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_executable(trading_tests 
        src/tests.cpp 
        src/order_book.cpp 
        src/matching_engine.cpp
    )
    target_include_directories(trading_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME AllTests COMMAND trading_tests)
endif()
