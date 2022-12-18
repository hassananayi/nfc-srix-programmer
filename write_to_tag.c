/*
 * Copyright 2022 Hassan ABBAS
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nfc/nfc.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "logging.h"
#include "nfc_utils.h"

int main(int argc, char *argv[], char *envp[]) {
    
    // Default options
    bool skip_confirmation = false;
    uint32_t eeprom_size = SRIX4K_EEPROM_SIZE;
    uint32_t eeprom_blocks_amount = SRIX4K_EEPROM_BLOCKS;
    set_verbose(false);


    // read config
    typedef struct {char key[20];char value[10];} settings;

    // file pointer variable for accessing the file
    FILE *file_config;

    // attempt to open config in read mode to read the file contents
    file_config = fopen("config", "r"); 

    // if the file failed to open, exit with an error message and status
    if (file_config == NULL){
        lerror("Cannot open config file \"%s\". Exiting...\n", file_config);
        exit(1);
    }

    // array of structs for storing the settings data from the file
    settings setting[100];

    // read will be used to ensure each line/record is read correctly
    int read = 0;

    int records = 0;

    // read all records from the file and store them into the settings array
    do {
    read = fscanf(file_config,"%20[^=]=%6[^;];\n", setting[records].key, setting[records].value); 

    if (read == 2) records++;

    if (read != 2 && !feof(file_config)){
        lerror("Config file format incorrect. Exiting...\n");
        exit(1);
    }

    // if there was an error reading from the file exit with an error message 
    // and status
    if (ferror(file_config)){
        lerror("Error reading Config file. Exiting...\n");
        exit(1);
        return 1;
    }

    } while (!feof(file_config));

    fclose(file_config);


    // Parse options

    //check tag type
    if (strcmp(setting[0].value, "512") == 0) {
        eeprom_size = SRI512_EEPROM_SIZE;
        eeprom_blocks_amount = SRI512_EEPROM_BLOCKS;
    }

    
    // check verbose
    if (strcmp(setting[2].value, "on") == 0) {
        set_verbose(true);
    }

    // skip_confirmation
    if (strcmp(setting[3].value, "on") == 0) {
        skip_confirmation = true;
    }


    // Initialize NFC
    nfc_context *context = NULL;
    nfc_device *reader = NULL;
    nfc_init(&context);
    if (context == NULL) {
        lerror("Unable to init libnfc. Exiting...\n");
        exit(1);
    }

    // Display libnfc version
    lverbose("libnfc version: %s\n", nfc_version());

    // Search for readers
    lverbose("Searching for readers... ");
    nfc_connstring connstrings[MAX_DEVICE_COUNT] = {};
    size_t num_readers = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);
    lverbose("found %zu.\n", num_readers);

    // Check if no readers are available
    if (num_readers == 0) {
        lerror("No readers available. Exiting...\n");
        close_nfc(context, reader);
        exit(1);
    }

    // Print out readers
    for (unsigned int i = 0; i < num_readers; i++) {
        if (i == num_readers - 1) {
            lverbose("└── ");
        } else {
            lverbose("├── ");
        }
        lverbose("[%d] %s\n", i, connstrings[i]);
    }
    lverbose("Opening %s...\n", connstrings[0]);

    // Open first reader
    reader = nfc_open(context, connstrings[0]);
    if (reader == NULL) {
        lerror("Unable to open NFC device. Exiting...\n");
        close_nfc(context, reader);
        exit(1);
    }

    // Set opened NFC device to initiator mode
    if (nfc_initiator_init(reader) < 0) {
        lerror("nfc_initiator_init => %s\n", nfc_strerror(reader));
        close_nfc(context, reader);
        exit(1);
    }

    lverbose("NFC reader: %s\n", nfc_device_get_name(reader));

    nfc_target target_key[MAX_TARGET_COUNT];

    /*
     * This is a known bug from libnfc.
     * To read ISO14443B2SR you have to initiate first ISO14443B to configure internal registers.
     *
     * https://github.com/nfc-tools/libnfc/issues/436#issuecomment-326686914
     */
    lverbose("Searching for ISO14443B targets... found %d.\n", nfc_initiator_list_passive_targets(reader, nmISO14443B, target_key, MAX_TARGET_COUNT));

    lverbose("Searching for ISO14443B2SR targets...");
    int ISO14443B2SR_targets = nfc_initiator_list_passive_targets(reader, nmISO14443B2SR, target_key, MAX_TARGET_COUNT);
    lverbose(" found %d.\n", ISO14443B2SR_targets);

    // Check for tags
    if (ISO14443B2SR_targets == 0) {
        printf("Waiting for tag...\n");

        // Infinite select for tag
        if (nfc_initiator_select_passive_target(reader, nmISO14443B2SR, NULL, 0, target_key) <= 0) {
            lerror("nfc_initiator_select_passive_target => %s\n", nfc_strerror(reader));
            close_nfc(context, reader);
            exit(1);
        }
    }


    char file_path[100];
    uint8_t *dump_bytes = malloc(sizeof(uint8_t) * eeprom_size);

    printf(YELLOW "\n>>> Enter file name: " RESET);
    scanf("%100s", file_path); 


    // Open file
    lverbose("Reading \"%s\"...\n", file_path);
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        lerror("Cannot open \"%s\". Exiting...\n", file_path);
        exit(1);
    }

    // Check file size
    int fd = fileno(fp);
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        lerror("Error doing fstat. Exiting...\n");
        exit(1);
    }
    if (file_stat.st_size < eeprom_size) {
        lerror("File wrong size, expected %llu but read %llu. Exiting...\n", eeprom_size, file_stat.st_size);
        exit(1);
    }

    // Read file
    fseek(fp, 0, SEEK_SET);
    if (fread(dump_bytes, eeprom_size, 1, fp) != 1) {
        lerror("Error encountered while reading file. Exiting...\n");
        exit(1);
    }
    fclose(fp);

    // Read EEPROM
    uint8_t *eeprom_bytes = malloc(sizeof(uint8_t) * eeprom_size);
    lverbose("Reading %d blocks...\n", eeprom_blocks_amount);
    for (int i = 0; i < eeprom_blocks_amount; i++) {
        uint8_t *current_block = eeprom_bytes + (i * 4);
        uint8_t block_bytes_read = nfc_srix_read_block(reader, current_block, i);

        // Check for errors
        if (block_bytes_read != 4) {
            lerror("Error while reading block %d. Exiting...\n", i);
            lverbose("Received %d bytes instead of 4.\n", block_bytes_read);
            close_nfc(context, reader);
            exit(1);
        }
    }

    // Preview write
    bool is_equal = true;
    for (uint8_t i = 7; i < eeprom_blocks_amount; i++) {
        uint32_t dump_block = dump_bytes[(i*4)+0] << 24u | dump_bytes[(i*4)+1] << 16u | dump_bytes[(i*4)+2] << 8u | dump_bytes[(i*4)+3];
        uint32_t eeprom_block = eeprom_bytes[(i*4)+0] << 24u | eeprom_bytes[(i*4)+1] << 16u | eeprom_bytes[(i*4)+2] << 8u | eeprom_bytes[(i*4)+3];

        if (dump_block != eeprom_block) {
            is_equal = false;
            printf("[%02X] %08X -> %08X\n", i, dump_block, eeprom_block);
        }
    }

    if (!is_equal) {
        // Ask for confirmation
        if (!skip_confirmation) {
            printf(YELLOW ">>> This action is irreversible. Are you sure? [Y/N]: " RESET);
            char c = 'n';
            scanf(" %c", &c);
            if (c != 'Y' && c != 'y') {
                printf("Exiting...\n");
                exit(0);
            }
        }

        // Ask for OTP area
        bool write_otp_area = true;
        if (!skip_confirmation) {
            printf(YELLOW ">>> Writing to OTP area do you want to continue? [Y/N]: " RESET);
            char c = 'n';
            scanf(" %c", &c);
            if (c != 'Y' && c != 'y') {
                write_otp_area = false;
            }
        }



        for (uint8_t i = 0; i < eeprom_blocks_amount; i++) {
            // Skip critical sectors
            if (!write_otp_area) {
                continue;
            }

            uint32_t dump_block = dump_bytes[(i*4)+0] << 24u | dump_bytes[(i*4)+1] << 16u | dump_bytes[(i*4)+2] << 8u | dump_bytes[(i*4)+3];
            uint32_t eeprom_block = eeprom_bytes[(i*4)+0] << 24u | eeprom_bytes[(i*4)+1] << 16u | eeprom_bytes[(i*4)+2] << 8u | eeprom_bytes[(i*4)+3];

            if (dump_block != eeprom_block) {
                nfc_write_block(reader, dump_block, i);
            }
        }
    } else {
        printf("This dump is already written to this NFC tag.\n");
    }

    // Close NFC
    close_nfc(context, reader);

    return 0;
}