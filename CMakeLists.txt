cmake_minimum_required(VERSION 3.14)
project(nfc-srix-programmer C)
set(CMAKE_C_STANDARD 99)

# Use PkgConfig to find libnfc
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNFC REQUIRED libnfc)
include_directories(${LIBNFC_INCLUDE_DIRS})
link_directories(${LIBNFC_LIBRARY_DIRS})
add_definitions(${LIBNFC_CFLAGS_OTHER})


# main
add_executable(nfc-srix main.c logging.c nfc_utils.c)
target_link_libraries(nfc-srix ${LIBNFC_LIBRARIES})



