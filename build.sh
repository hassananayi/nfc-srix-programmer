#!/usr/bin/env bash

# Create build folder
mkdir build

# Copy config file
cp ./config build/config

cd build

# Create commands folder
mkdir commands

TEMP_DIR=$(mktemp -d build.XXXXXXXX)
cd "${TEMP_DIR}"

# Build
cmake ../../
make

# Copy executables
mv nfc-srix-programmer ../
mv read_eeprom_content ../commands/
mv read_tag_info ../commands/
mv write_eeprom_to_file ../commands/
mv read_eeprom_file ../commands/
mv modify_block ../commands/
mv write_to_tag ../commands/
mv otp_reset ../commands/

# Cleanup
cd ../
rm -fr "${TEMP_DIR}"
