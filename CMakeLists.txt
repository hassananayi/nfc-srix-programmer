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
add_executable(nfc-srix-programmer main.c logging.c nfc_utils.c)
target_link_libraries(nfc-srix-programmer ${LIBNFC_LIBRARIES})

# Read EEPROM content
add_executable(read_eeprom_content read_eeprom_content.c logging.c nfc_utils.c)
target_link_libraries(read_eeprom_content ${LIBNFC_LIBRARIES})

# Read Tag info
add_executable(read_tag_info read_tag_info.c logging.c nfc_utils.c)
target_link_libraries(read_tag_info ${LIBNFC_LIBRARIES})

# Write EEPROM to a file
add_executable(write_eeprom_to_file write_eeprom_to_file.c logging.c nfc_utils.c)
target_link_libraries(write_eeprom_to_file ${LIBNFC_LIBRARIES})

# Read EEPROM from a file
add_executable(read_eeprom_file read_eeprom_file.c logging.c nfc_utils.c)
target_link_libraries(read_eeprom_file ${LIBNFC_LIBRARIES})

# modify_block
add_executable(modify_block modify_block.c logging.c nfc_utils.c)
target_link_libraries(modify_block ${LIBNFC_LIBRARIES})

# Write EEPROM file to NFC tag
add_executable(write_to_tag write_to_tag.c logging.c nfc_utils.c)
target_link_libraries(write_to_tag ${LIBNFC_LIBRARIES})

# reset OTP
add_executable(otp_reset otp_reset.c logging.c nfc_utils.c)
target_link_libraries(otp_reset ${LIBNFC_LIBRARIES})



