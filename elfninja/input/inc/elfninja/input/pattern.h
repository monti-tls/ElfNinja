/*
 * This file is part of elfninja
 * Copyright (C) 2017  Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ELFNINJA_INPUT_PATTERN_H__
#define __ELFNINJA_INPUT_PATTERN_H__

#include "elfninja/core/error.h"
#include "elfninja/core/elf.h"

int enji_pattern_match(enj_elf_shdr* section, const char* pattern, enj_error** err);
enj_elf_shdr* enji_pattern_match_unique(enj_elf* elf, const char* pattern, enj_error** err);

int enji_pattern_is_index(const char* pattern, enj_error** err);

#endif // __ELFNINJA_INPUT_PATTERN_H__
