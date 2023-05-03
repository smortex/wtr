add_library(libwtr config.c database.c utils.c)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 sqlite3)
target_link_libraries(libwtr PkgConfig::deps)
