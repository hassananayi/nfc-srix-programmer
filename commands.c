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

    
// Initialize NFC
nfc_context *context = NULL;
nfc_device *reader = NULL;
void initialize_nfc(){

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

}

// Read EEPROM content
void read_eeprom_content() {

    // Initialize NFC
    initialize_nfc();


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

        printf("[%02X] ", i);
        printf("%02X %02X %02X %02X ", current_block[0], current_block[1], current_block[2], current_block[3]);
        printf(DIM);
        printf("--- %s\n", srix_get_block_type(i));
        printf(RESET);
    }




    // Close NFC
    close_nfc(context, reader);
    
}

// Read tag info
void read_tag_info() {

    // Initialize NFC
    initialize_nfc();

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

}

// Write EEPROM to a file
void write_eeprom_to_file() {
    
    // Initialize NFC
    initialize_nfc();


    // ask for file path
    char output_path[100];

    printf(YELLOW "\n>>> Enter file name: " RESET);
    scanf("%100s", output_path); 

    // Check if file already exists
    FILE *file = fopen(output_path, "r");
    if (file) {
        fclose(file);

        // Ask for confirmation
        if (!skip_confirmation) {
            printf("\"%s\" already exists.\n", output_path);
            printf(YELLOW "\n>>> Do you want to overwrite it? [Y/N]:" RESET);
            char c = 'n';
            scanf(" %c", &c);
            if (c != 'Y' && c != 'y') {
                printf("Exiting...\n");
               exit(0);
                
            }
        }
    }


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

        printf("[%02X] ", i);
        printf("%02X %02X %02X %02X ", current_block[0], current_block[1], current_block[2], current_block[3]);
        printf(DIM);
        printf("--- %s\n", srix_get_block_type(i));
        printf(RESET);
    }


    // export dump to file
    FILE *fp = fopen(output_path, "w");
    fwrite(eeprom_bytes, eeprom_size, 1, fp);
    fclose(fp);

    printf("Written dump to \"%s\".\n", output_path);


    // Close NFC
    close_nfc(context, reader);

}

// Read EEPROM file
void read_eeprom_file() {  

    // Start for read dump file
    char file_path[100];
    uint8_t *eeprom_bytes = malloc(sizeof(uint8_t) * eeprom_size);

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
    if (fread(eeprom_bytes, eeprom_size, 1, fp) != 1) {
        lerror("Error encountered while reading file. Exiting...\n");
        exit(1);
    }
    fclose(fp);

    for(int i = 0; i < eeprom_blocks_amount; i++) {
            uint8_t *block = eeprom_bytes + (i * 4);
            printf("[%02X] %02X %02X %02X %02X" DIM " --- %s\n" RESET, i, block[0], block[1], block[2], block[3], srix_get_block_type(i));
    }

}

// Modfiy Block
void modfiy_block() {

    // Initialize NFC
    initialize_nfc();

    // Ask for block adress

    printf(YELLOW ">>> Enter Block address [ex 0A]:" RESET); 
    
    unsigned int block_addr;
    scanf("%x", &block_addr);

    /*
  if (!block_addr > 2) {
       lerror("Invalid block address %d. Exiting...\n", 1);
        exit(0);
    } 
 
    */


    // Read target blocks
    printf("Reading Block...\n");
 
    uint8_t block_bytes[4] = {};
    uint8_t block_bytes_read = nfc_srix_read_block(reader, block_bytes, block_addr);

    // Check for errors
    if (block_bytes_read != 4) {
        lerror("Error while reading block %d. Exiting...\n", 1);
        lverbose("Received %d bytes instead of 4.\n", block_bytes_read);
        close_nfc(context, reader);
        exit(1);
    }


    // print block 
    printf("[%02X] %02X %02X %02X %02X" DIM " --- %s\n" RESET, block_addr, block_bytes[0], block_bytes[1], block_bytes[2], block_bytes[3], srix_get_block_type(block_addr));



    // Ask for new value for the block

    printf(YELLOW ">>> Enter hexadecimal value without Space or \"0x\": " RESET); 

    unsigned int block_new_value;
    scanf("%8x", &block_new_value);
	if (!block_new_value) {
        lerror("Invalid hexadecimal value %d. Exiting...\n", 1);
        exit(0);
    } 


    // info
    //printf(BOLD NOTE "Note on writing tags!\n" RESET);
    lwarning("Note on writing tags!\n");
    printf("Compliant ST SRx tags have some blocks that, once changed, \ncannot be changed back to their original value. \nExample Counters Blocks 5 and 6. \nBefore writing a tag, make sure you're aware of this.\n");


    printf(YELLOW ">>> This action is irreversible. Are you sure? [Y/N]: " RESET);
 
    char c = 'n';
    scanf(" %c", &c);
    if (c != 'Y' && c != 'y') {
        printf("\nExiting...\n");
        exit(0);
    }


    // Write Block 
     nfc_write_block(reader, block_new_value, block_addr);
 

    // Close NFC
    close_nfc(context, reader);
}

// Write to NFC Tag
void write_to_tag() {
    
    // Initialize NFC
    initialize_nfc();

    // Ask for file name
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

}

// OTP Blocks Reset
void otp_reset() {
   
    // Initialize NFC
    initialize_nfc();

    // Read necessary blocks
    uint32_t otp_blocks[6] = {};
    printf("Reading OTP blocks...\n");
    for (uint8_t i = 0; i < 6; i++) {
        // Skip block 0x05
        if (i == 5) i++;

        uint8_t block_bytes[4] = {};
        uint8_t block_bytes_read = nfc_srix_read_block(reader, block_bytes, i);

        // Check for errors
        if (block_bytes_read != 4) {
            lerror("Error while reading block %d. Exiting...\n", i);
            lverbose("Received %d bytes instead of 4.\n", block_bytes_read);
            close_nfc(context, reader);
            exit(1);
        }

        otp_blocks[i] = block_bytes[0] << 24u | block_bytes[1] << 16u | block_bytes[2] << 8u | block_bytes[3];

        //printf("%08X\n", otp_blocks[i]);
        printf("[%02X] %08X \n", i, otp_blocks[i]);
    }

    // Check if already reset
    bool otp_already_reset = true;
    for (uint8_t i = 0; i < 5; i++) {
        if (otp_blocks[i] != 0xFFFFFFFF) otp_already_reset = false;
    }

    if (otp_already_reset) {
        printf("OTP area already reset.\n");
        close_nfc(context, reader);
        exit(0);
    }

    uint32_t block_6 = (otp_blocks[5] << 24u) | (otp_blocks[5] << 16u) | (otp_blocks[5] << 8u) | otp_blocks[5];
    printf("OTP resets available: %u\n", block_6 >> 21u);

    block_6 -= (1u << 21u);
    printf("OTP resets remaining after this operation: %u\n", block_6 >> 21u);

    // Reverse
    block_6 = (otp_blocks[5] << 24u) | (otp_blocks[5] << 16u) | (otp_blocks[5] << 8u) | otp_blocks[5];

    // Show differences
    printf("[%02X] %08X -> FFFFFFFF\n", 0x00, otp_blocks[0]);
    printf("[%02X] %08X -> FFFFFFFF\n", 0x01, otp_blocks[1]);
    printf("[%02X] %08X -> FFFFFFFF\n", 0x02, otp_blocks[2]);
    printf("[%02X] %08X -> FFFFFFFF\n", 0x03, otp_blocks[3]);
    printf("[%02X] %08X -> FFFFFFFF\n", 0x04, otp_blocks[4]);
    printf("[%02X] %08X -> %08X\n", 0x06, otp_blocks[5], block_6);

    // Ask for confirmation
    if (!skip_confirmation) {
        printf(YELLOW ">>> This action is irreversible. Are you sure? [Y/N]: " RESET);
        char c = 'n';
        scanf(" %c", &c);
        if (c != 'Y' && c != 'y') {
            printf("\nExiting...\n");
            exit(0);
        }
    }

    // Write Block 06 first to trigger an Auto erase cycle
    nfc_write_block(reader, block_6, 0x06);
    nfc_write_block(reader, 0xFFFFFFFF, 0x00);
    nfc_write_block(reader, 0xFFFFFFFF, 0x01);
    nfc_write_block(reader, 0xFFFFFFFF, 0x02);
    nfc_write_block(reader, 0xFFFFFFFF, 0x03);
    nfc_write_block(reader, 0xFFFFFFFF, 0x04);

    // Close NFC
    close_nfc(context, reader);
}

// hellp
void print_options(const char *executable) {
    printf("Usage: %s [-v] [-y] [-t x4k|512]\n", executable);
    printf("\nOptions:\n");
    printf("  -v           enable verbose - print debugging data\n");
    printf("  -y           answer YES to all questions\n");
    printf("  -t x4k|512   select SRIX4K or SRI512 tag type [default: x4k]\n");
}
