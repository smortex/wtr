find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
flex_target(cmd_lexer ${CMAKE_CURRENT_SOURCE_DIR}/cmd_lexer.l ${CMAKE_CURRENT_BINARY_DIR}/cmd_lexer.c DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/cmd_lexer.h)
bison_target(cmd_parser ${CMAKE_CURRENT_SOURCE_DIR}/cmd_parser.y ${CMAKE_CURRENT_BINARY_DIR}/cmd_parser.c COMPILE_FLAGS "-d -v")
add_flex_bison_dependency(cmd_lexer cmd_parser)
add_executable(wtr wtr.c ${BISON_cmd_parser_OUTPUTS} ${FLEX_cmd_lexer_OUTPUTS})
find_package(PkgConfig REQUIRED)
target_link_libraries(wtr libwtr m)
install(TARGETS wtr DESTINATION bin)
install(FILES wtr.1 DESTINATION man/man1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
