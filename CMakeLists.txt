cmake_minimum_required(VERSION "3.27")
project("wtr" VERSION "1.0.0")
include(CTest)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	find_program(CLANG_TIDY "clang-tidy")

	if(CLANG_TIDY)
		set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY} "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
	endif()
endif()

add_compile_options(-Wall -Wextra -Werror)
add_definitions(-D_GNU_SOURCE)

add_subdirectory("libwtr")
add_subdirectory("wtr")
add_subdirectory("wtrd")

add_custom_target(style astyle --options=${CMAKE_SOURCE_DIR}/.astylerc --exclude=${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/\\*.c" "${CMAKE_SOURCE_DIR}/\\*.h")
