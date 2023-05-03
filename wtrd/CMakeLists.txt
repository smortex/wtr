add_executable(wtrd wtrd.c)
find_package(PkgConfig REQUIRED)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
target_sources(wtrd PUBLIC freebsd.c)
find_library(wtrd procstat)
target_link_libraries(wtrd procstat)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
target_sources(wtrd PUBLIC linux.c)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET libbsd-overlay)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 sqlite3)
target_link_libraries(wtrd PkgConfig::deps libwtr)
install(TARGETS wtrd DESTINATION bin)
