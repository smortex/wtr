add_library(libwtr config.c database.c libwtr.c utils.c)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 sqlite3)
target_link_libraries(libwtr PkgConfig::deps)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
target_sources(libwtr PRIVATE freebsd.c)
find_library(libwtr procstat)
target_link_libraries(libwtr procstat)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
target_sources(libwtr PRIVATE linux.c)
pkg_check_modules(libbsd REQUIRED IMPORTED_TARGET libbsd-overlay)
target_link_libraries(libwtr PkgConfig::libbsd)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

add_subdirectory("tests")
