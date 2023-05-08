add_executable(wtrd wtrd.c)
find_package(PkgConfig REQUIRED)
target_link_libraries(wtrd libwtr)
install(TARGETS wtrd DESTINATION bin)
install(FILES wtrd.1 DESTINATION man/man1)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
target_link_libraries(wtrd procstat)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
pkg_check_modules(libbsd REQUIRED IMPORTED_TARGET libbsd-overlay)
target_link_libraries(wtrd PkgConfig::libbsd)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
