cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(psionic)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_library(psionic src/PSEngine.cpp src/CompiledGame.cpp src/Compiler.cpp src/ParsedGame.cpp src/Parser.cpp src/PSLogger.cpp src/TextProvider.cpp)
add_executable(psionic_main src/main.cpp)
target_link_libraries(psionic_main psionic)
