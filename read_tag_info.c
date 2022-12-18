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
#include <stdbool.h>
#include <nfc/nfc.h>
#include <inttypes.h>
#include "logging.h"
#include "nfc_utils.h"


int main(int argc, char *argv[], char *envp[]) {
   

    // Default options
    char *output_path = NULL;
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

    // Read UID
    uint8_t uid_rx_bytes[MAX_RESPONSE_LEN] = {};
    uint8_t uid_bytes_read = nfc_srix_get_uid(reader, uid_rx_bytes);

    // Check for errors
    if (uid_bytes_read != 8) {
        lerror("Error while reading UID. Exiting...\n");
        lverbose("Received %d bytes instead of 8.\n", uid_bytes_read);
        close_nfc(context, reader);
        exit(1);
    }

    // Convert to uint64
    uint64_t uid = (uint64_t) uid_rx_bytes[0] | (uint64_t) uid_rx_bytes[1] << 8u |(uint64_t) uid_rx_bytes[2] << 16u | (uint64_t) uid_rx_bytes[3] << 24u |(uint64_t) uid_rx_bytes[4] << 32u |(uint64_t) uid_rx_bytes[5] << 40u |(uint64_t) uid_rx_bytes[6] << 48u | (uint64_t) uid_rx_bytes[7] << 56u;
    uint64_t uid_fix_reding = (uint64_t) uid_rx_bytes[7] | (uint64_t) uid_rx_bytes[6] << 8u |(uint64_t) uid_rx_bytes[5] << 16u | (uint64_t) uid_rx_bytes[4] << 24u |(uint64_t) uid_rx_bytes[3] << 32u |(uint64_t) uid_rx_bytes[2] << 40u |(uint64_t) uid_rx_bytes[1] << 48u | (uint64_t) uid_rx_bytes[0] << 56u;

    // Print UID
 
    printf("UID: %016" PRIX64 "\n", uid_fix_reding);

    // Convert uint64 to binary char array
    char uid_binary[65] = {};
    for (unsigned int i = 0; i < sizeof(uid); i++) {
        uint8_t tmp = (uid >> (sizeof(uid) - 1 - i) * 8u) & 0xFFu;
        sprintf(uid_binary + i * 8 + 0, "%c", tmp & 0x80u ? '1' : '0');
        sprintf(uid_binary + i * 8 + 1, "%c", tmp & 0x40u ? '1' : '0');
        sprintf(uid_binary + i * 8 + 2, "%c", tmp & 0x20u ? '1' : '0');
        sprintf(uid_binary + i * 8 + 3, "%c", tmp & 0x10u ? '1' : '0');
        sprintf(uid_binary + i * 8 + 4, "%c", tmp & 0x08u ? '1' : '0');
        sprintf(uid_binary + i * 8 + 5, "%c", tmp & 0x04u ? '1' : '0');
        sprintf(uid_binary + i * 8 + 6, "%c", tmp & 0x02u ? '1' : '0');
        sprintf(uid_binary + i * 8 + 7, "%c", tmp & 0x01u ? '1' : '0');
    }

    printf("├── Prefix: %02" PRIX64 "\n", uid >> 56u);
    printf("├── IC manufacturer code: %02" PRIX64, (uid >> 48u) & 0xFFu);
    switch ((uid >> 48u) & 0xFFu) {
        case 0x02:
            printf(" (STMicroelectronics)\n");
            break;
        default:
            printf(" (unknown)\n");
    }

    // Print 6bit IC code
    char ic_code[7] = {};
    memcpy(ic_code, uid_binary + 16, 6);
    printf("├── IC code: %s [%" PRIu64 "]\n", ic_code, (uid >> 42u) & 0x7u);


    // Print 42bit unique serial number
    char unique_serial_number[43] = {};
    memcpy(unique_serial_number, uid_binary + 22, 42);

    // Print UID Binary
    printf("├── 42bit UID Binary:  %s\n", unique_serial_number);

    printf("└── Serial number: %" PRIu64 "\n" , uid & 0x3FFFFFFFFFFu);


    
    // Print System blocks
    uint8_t system_block_bytes[4] = {};
    uint8_t system_block_bytes_read = nfc_srix_read_block(reader, system_block_bytes, 0xFF);

    // Check for errors
    if (system_block_bytes_read != 4) {
        lerror("Error while reading block %d. Exiting...\n", 0xFF);
        lverbose("Received %d bytes instead of 4.\n", system_block_bytes_read);
        close_nfc(context, reader);
        exit(1);
    }

    uint32_t system_block = system_block_bytes[3] << 24u | system_block_bytes[2] << 16u | system_block_bytes[1] << 8u | system_block_bytes[0];

    printf("\nSystem block: %02X %02X %02X %02X\n", system_block_bytes[3], system_block_bytes[2], system_block_bytes[1], system_block_bytes[0]);
    printf("├── CHIP_ID: %02X\n", system_block_bytes[0]);
    printf("├── ST reserved: %02X%02X\n", system_block_bytes[1], system_block_bytes[2]);
    printf("└── OTP_Lock_Reg:\n");
    for (uint8_t i = 24; i < 32; i++) {
        if (i == 31) {
            printf("    └── b%d = %d - ", i, (system_block >> i) & 1u);
        } else {
            printf("    ├── b%d = %d - ", i, (system_block >> i) & 1u);
        }

        if (i == 24) {
            printf("Block 07 and 08 are ");
        } else {
            printf("Block %02X is ", i - 16);
        }

        if (((system_block >> i) & 1u) == 0) {
            printf(RED);
            printf("LOCKED\n");
            printf(RESET);
        } else {
            printf(GREEN);
            printf("unlocked\n");
            printf(RESET);
        }
        }
    // Close NFC
    close_nfc(context, reader);

    return 0;
}