cmake_minimum_required(VERSION 3.10.0)
project(main VERSION 0.1.0 LANGUAGES C CXX)

find_package(Tesseract CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE cxxopts::cxxopts Tesseract::libtesseract)
target_compile_features(main PRIVATE cxx_std_26)
