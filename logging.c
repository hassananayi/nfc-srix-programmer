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
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "logging.h"

bool verbose_status = false;
int verbosity_level = 0;

uint32_t eeprom_size = 512;
uint32_t eeprom_blocks_amount = 128;
bool skip_confirmation = false;


void set_eeprom_size(uint32_t eeprom_size_value) {
    eeprom_size = eeprom_size_value;
}
void set_eeprom_blocks_amount(uint32_t eeprom_blocks_value) {
    eeprom_blocks_amount = eeprom_blocks_value;
}
void set_skip_confirmation(bool value){
   skip_confirmation = value;
}

void set_verbose(bool setting) {
    verbose_status = setting;
}

void set_verbosity(int level) {
    verbosity_level = level;
}

int lverbose(const char * restrict format, ...) {
    if (!verbose_status) {
        return 0;
    }

    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}

int lverbose_lvl(int level, const char * restrict format, ...) {
    if (level < verbosity_level) {
        return 0;
    }

    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}

int lerror(const char * restrict format, ...) {
    fprintf(stderr, BOLD);
    fprintf(stderr, RED);
    fprintf(stderr, "ERROR: ");
    fprintf(stderr, RESET);

    va_list args;
    va_start(args, format);
    int ret = vfprintf(stderr, format, args);
    va_end(args);

    return ret;
}

int lwarning(const char * restrict format, ...) {
    fprintf(stderr, BOLD);
    fprintf(stderr, YELLOW);
    fprintf(stderr, "WARNING: ");
    fprintf(stderr, RESET);

    va_list args;
    va_start(args, format);
    int ret = vfprintf(stderr, format, args);
    va_end(args);

    return ret;
}