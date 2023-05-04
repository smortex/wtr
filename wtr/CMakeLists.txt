add_executable(wtr wtr.c)
find_package(PkgConfig REQUIRED)
target_link_libraries(wtr libwtr)
install(TARGETS wtr DESTINATION bin)
