cmake_minimum_required(VERSION 3.20)

set (PostgreSQL_ADDITIONAL_VERSIONS "16")

project (Homework_5)

add_subdirectory("C:\\Users\\Kir17\\source\\repos\\libpqxx-7.9.1" pqxx-build)


add_executable( Homework_5 main.cpp main.cpp)

target_compile_features (Homework_5 PRIVATE cxx_std_17)

target_link_libraries (Homework_5 pqxx) 