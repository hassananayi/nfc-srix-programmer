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

#ifndef LOGGING_H
#define LOGGING_H

// Foreground
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define NOTE "\033[36m"
#define BLUE "\e[34m"

// Styles
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"

extern bool verbose_status;
extern int verbosity_level;

extern uint32_t eeprom_size;
extern uint32_t eeprom_blocks_amount;
extern bool skip_confirmation;

void set_eeprom_size(uint32_t);
void set_eeprom_blocks_amount(uint32_t);
void set_skip_confirmation(bool);

void set_verbose(bool);
void set_verbosity(int);
int lverbose(const char * restrict, ...);
int lverbose_lvl(int, const char * restrict, ...);
int lerror(const char * restrict, ...);
int lwarning(const char * restrict, ...);

#endif // LOGGING_H
