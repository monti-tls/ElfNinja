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

#include "elfninja/core/error.h"
#include "elfninja/core/malloc.h"
#include "elfninja/core/blob.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/symtab.h"
#include "elfninja/core/note.h"
#include "elfninja/core/note_gnu.h"
#include "elfninja/core/dynamic.h"
#include "elfninja/core/trait/layout.h"
