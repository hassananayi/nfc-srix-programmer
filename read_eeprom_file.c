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
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <nfc/nfc.h>
#include <sys/stat.h>
#include "logging.h"
#include "nfc_utils.h"


int main(int argc, char *argv[], char *envp[]) {
    
    // Default options
    int print_columns = 1;
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
    if (strcmp(setting[0].value, "512") == 0) {
        eeprom_size = SRI512_EEPROM_SIZE;
        eeprom_blocks_amount = SRI512_EEPROM_BLOCKS;
    }

    print_columns = (int) strtol(setting[1].value, NULL, 10);

    // Check columns
    if (print_columns != 1 && print_columns != 2) {
        lwarning("Invalid number of columns. Input is %d, but must be either 1 or 2.\nUsing default value.\n", print_columns);
        print_columns = 1;
    }
    
    // check verbose
    if (strcmp(setting[2].value, "on") == 0) {
        set_verbose(true);
    }




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

    if (print_columns == 1) { // Single column print
        for (int i = 0; i < eeprom_blocks_amount; i++) {
            uint8_t *block = eeprom_bytes + (i * 4);
            printf("[%02X] %02X %02X %02X %02X" DIM " --- %s\n" RESET, i, block[0], block[1], block[2], block[3], srix_get_block_type(i));
        }
    } else { // Double column print
        for (int i = 0; i < eeprom_blocks_amount; i += 2) {
            uint8_t *block_1 = eeprom_bytes + (i * 4);
            uint8_t *block_2 = eeprom_bytes + ((i+1) * 4);
            printf(DIM "%19s --- " RESET "[%02X] %02X %02X %02X %02X  %02X %02X %02X %02X [%02X]" DIM " --- %s\n" RESET,
                    srix_get_block_type(i), i, block_1[0], block_1[1], block_1[2], block_1[3], block_2[0], block_2[1], block_2[2], block_2[3], i+1, srix_get_block_type(i+1));
        }
    }

    return 0;
}