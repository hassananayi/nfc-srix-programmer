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
#include "commands.c"

int main(int argc, char *argv[], char *envp[]){

  //Defult options
  set_skip_confirmation(false);
  set_eeprom_size(SRIX4K_EEPROM_SIZE);
  set_eeprom_blocks_amount(SRIX4K_EEPROM_BLOCKS);
  set_verbose(false);

  // Parse options
  int opt = 0;
  while ((opt = getopt(argc, argv, "hvyt:")) != -1) {
      switch (opt) {
          case 'v': set_verbose(true); break;
          case 'y':set_skip_confirmation(true); break;
          case 't':
              if (strcmp(optarg, "512") == 0) {
                  set_eeprom_size(SRI512_EEPROM_SIZE);
                  set_eeprom_blocks_amount(SRI512_EEPROM_BLOCKS);
              }
              break;
          
      }
  }

  int choice = 0;

    while(true){

        printf("\n\n[NFC SRIX Programmer]\n\n");
        
        printf(GREEN "1) " RESET "Search for NFC devices\n" ); 
        printf(GREEN "2) " RESET "Read EEPROM content\n" );
        printf(GREEN "3) " RESET "Read NFC Tag information\n" );
        printf(GREEN "4) " RESET "Write EEPROM to a file\n" );
        printf(GREEN "5) " RESET "Read EEPROM from a file\n" );
        printf(GREEN "6) " RESET "Modify block manually\n" );
        printf(GREEN "7) " RESET "Write EEPROM file to NFC tag\n" );
        printf(GREEN "8) " RESET "Reset OTP Blocks\n" );
        printf(GREEN "9) " RESET "Help\n" );
        printf(GREEN "0) " RESET "Exit\n" );  

        printf(YELLOW "\n>>> Choose an option: " RESET);
        scanf("%d", &choice); 

        switch (choice){
            case 1: system("nfc-list"); break;
            case 2: read_eeprom_content(); break;  
            case 3: read_tag_info(); break;
            case 4: write_eeprom_to_file(); break;
            case 5: read_eeprom_file(); break;
            case 6: modfiy_block();break;
            case 7: write_to_tag(); break;
            case 8: otp_reset(); break;
            case 9: print_options(argv[0]); break;        
            case 0: exit(0);
        }

    }

  return 0;
}