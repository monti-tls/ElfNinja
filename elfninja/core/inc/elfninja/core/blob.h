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

#ifndef __ELFNINJA_CORE_BLOB_H__
#define __ELFNINJA_CORE_BLOB_H__

#include "elfninja/core/error.h"

#include <stddef.h>

struct enj_blob_anchor;
struct enj_blob_cursor;

typedef struct enj_blob
{
    unsigned char* buffer;
    size_t buffer_size;
    size_t chunks;
    size_t chunk_size;

    struct enj_blob_anchor* anchors;
    struct enj_blob_anchor* last_anchor;

    struct enj_blob_cursor* cursors;
    struct enj_blob_cursor* last_cursor;
} enj_blob;

typedef struct enj_blob_anchor
{
    struct enj_blob* blob;

    int valid;
    int is_cursor_end;
    size_t pos;

    struct enj_blob_anchor* prev;
    struct enj_blob_anchor* next;
} enj_blob_anchor;

typedef struct enj_blob_cursor
{
    struct enj_blob* blob;

    enj_blob_anchor* start;
    enj_blob_anchor* end;

    int valid;
    int shrank;
    int grown;
    size_t length;

    struct enj_blob_cursor* prev;
    struct enj_blob_cursor* next;
} enj_blob_cursor;

enj_blob* enj_blob_create(enj_error** err);
void enj_blob_delete(enj_blob* blob);

enj_blob_anchor* enj_blob_new_anchor(enj_blob* blob, size_t pos, enj_error** err);
int enj_blob_remove_anchor(enj_blob* blob, enj_blob_anchor* anchor, enj_error** err);

enj_blob_cursor* enj_blob_new_cursor(enj_blob* blob, size_t pos, size_t length, enj_error** err);
int enj_blob_remove_cursor(enj_blob* blob, enj_blob_cursor* cursor, enj_error** err);

int enj_blob_read(enj_blob* blob, size_t start, void* ptr, size_t length, enj_error** err);
int enj_blob_write(enj_blob* blob, size_t start, void const* ptr, size_t length, enj_error** err);
int enj_blob_set(enj_blob* blob, size_t start, char value, size_t count, enj_error** err);
int enj_blob_insert(enj_blob* blob, size_t start, void const* ptr, size_t length, enj_error** err);
int enj_blob_remove(enj_blob* blob, size_t start, size_t length, enj_error** err);
int enj_blob_move(enj_blob* blob, size_t src, size_t dest, size_t length, enj_error** err);

int enj_blob__update_cursors(enj_blob* blob, enj_error** err);
int enj_blob__resize(enj_blob* blob, size_t new_size, enj_error** err);
int enj_blob__grow(enj_blob* blob, enj_error** err);
int enj_blob__shrink(enj_blob* blob, enj_error** err);

#endif // __ELFNINJA_CORE_BLOB_H__
