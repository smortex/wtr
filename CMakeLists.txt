cmake_minimum_required(VERSION "3.18")
project("wtr")

add_compile_options(-Wall -Wextra -Werror)
add_definitions(-D_GNU_SOURCE)

add_subdirectory("libwtr")
add_subdirectory("wtr")
add_subdirectory("wtrd")
