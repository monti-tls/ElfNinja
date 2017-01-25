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

#include "elfninja/core/blob.h"
#include "elfninja/core/malloc.h"

#include <string.h>
#include <unistd.h>

enj_blob* enj_blob_create(enj_error** err)
{
    enj_blob* blob = enj_malloc(sizeof(enj_blob));
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    blob->buffer = 0;
    blob->buffer_size = 0;
    blob->chunks = 0;
    blob->chunk_size = 256;
    blob->anchors = 0;
    blob->last_anchor = 0;
    blob->cursors = 0;
    blob->last_cursor = 0;

    return blob;
}

void enj_blob_delete(enj_blob* blob)
{
    if (!blob)
        return;

    for (enj_blob_anchor* anchor = blob->anchors; anchor; )
    {
        enj_blob_anchor* next = anchor->next;
        enj_free(anchor);
        anchor = next;
    }

    for (enj_blob_cursor* cursor = blob->cursors; cursor; )
    {
        enj_blob_cursor* next = cursor->next;
        enj_free(cursor);
        cursor = next;
    }

    enj_free(blob->buffer);
    enj_free(blob);
}

enj_blob_anchor* enj_blob_new_anchor(enj_blob* blob, size_t pos, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    if (pos > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return 0;
    }

    enj_blob_anchor* anchor = enj_malloc(sizeof(enj_blob_anchor));
    if (!anchor)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    anchor->blob = blob;
    anchor->valid = 1;
    anchor->is_cursor_end = 0;
    anchor->pos = pos;
    anchor->prev = blob->last_anchor;
    anchor->next = 0;

    if (anchor->prev)
        anchor->prev->next = anchor;
    else
        blob->anchors = anchor;

    blob->last_anchor = anchor;

    return anchor;
}

int enj_blob_remove_anchor(enj_blob* blob, enj_blob_anchor* anchor, enj_error** err)
{
    if (!blob || !anchor || anchor->blob != blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    if (anchor->prev)
        anchor->prev->next = anchor->next;
    else
        blob->anchors = anchor->next;

    if (anchor->next)
        anchor->next->prev = anchor->prev;
    else
        blob->last_anchor = anchor->prev;

    enj_free(anchor);

    return 0;
}

enj_blob_cursor* enj_blob_new_cursor(enj_blob* blob, size_t pos, size_t length, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    if (pos > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return 0;
    }

    enj_blob_cursor* cursor = enj_malloc(sizeof(enj_blob_cursor));
    if (!cursor)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    cursor->blob = blob;

    if (!(cursor->start = enj_blob_new_anchor(blob, pos, err)) ||
        !(cursor->end = enj_blob_new_anchor(blob, pos + length, err)))
    {
        enj_blob_remove_anchor(blob, cursor->start, err);
        enj_free(cursor);
        return 0;
    }

    cursor->end->is_cursor_end = 1;

    cursor->valid = 1;
    cursor->shrank = 0;
    cursor->grown = 0;
    cursor->length = length;
    cursor->prev = blob->last_cursor;
    cursor->next = 0;

    if (cursor->prev)
        cursor->prev->next = cursor;
    else
        blob->cursors = cursor;

    blob->last_cursor = cursor;

    return cursor;
}

int enj_blob_remove_cursor(enj_blob* blob, enj_blob_cursor* cursor, enj_error** err)
{
    if (!blob || !cursor || cursor->blob != blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    if (enj_blob_remove_anchor(blob, cursor->start, err) < 0 ||
        enj_blob_remove_anchor(blob, cursor->end, err) < 0)
    {
        return -1;
    }

    if (cursor->prev)
        cursor->prev->next = cursor->next;
    else
        blob->cursors = cursor->next;

    if (cursor->next)
        cursor->next->prev = cursor->prev;
    else
        blob->last_cursor = cursor->prev;

    enj_free(cursor);

    return 0;
}

int enj_blob_read(enj_blob* blob, size_t start, void* ptr, size_t length, enj_error** err)
{
    if (!blob || !ptr)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!length)
        return 0;

    if (start + length > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return -1;
    }

    memcpy(ptr, blob->buffer + start, length);

    return 0;
}

int enj_blob_write(enj_blob* blob, size_t start, void const* ptr, size_t length, enj_error** err)
{
    if (!blob || !ptr)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!length)
        return 0;

    if (start + length > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return -1;
    }

    memcpy(blob->buffer + start, ptr, length);

    return 0;
}

int enj_blob_set(enj_blob* blob, size_t start, char value, size_t count, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!count)
        return 0;

    if (start + count > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return -1;
    }

    memset(blob->buffer + start, value, count);

    return 0;
}

int enj_blob_insert(enj_blob* blob, size_t start, void const* ptr, size_t length, enj_error** err)
{
    if (!blob || !ptr)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!length)
        return 0;

    if (start > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return -1;
    }

    if (enj_blob__resize(blob, blob->buffer_size + length, err) < 0)
        return -1;

    if (start + length < blob->buffer_size)
    {
        size_t beg = start;
        size_t end = blob->buffer_size - length - 1;

        for (size_t i = end; ; --i)
        {
            blob->buffer[i + length] = blob->buffer[i];

            if (i == beg)
                break;
        }
    }

    memcpy(blob->buffer + start, ptr, length);

    for (enj_blob_anchor* anchor = blob->anchors; anchor; anchor = anchor->next)
    {
        if (!anchor->valid)
            continue;

        // if (anchor->pos >= start)
        // if (anchor->pos = start || (!anchor->is_cursor_end && anchor->pos == start))
        if (anchor->pos > start || (anchor->is_cursor_end && anchor->pos == start))
        {
            anchor->pos += length;
        }
    }

    if (enj_blob__update_cursors(blob, err) < 0)
        return -1;

    return 0;
}

int enj_blob_remove(enj_blob* blob, size_t start, size_t length, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!length)
        return 0;

    if (start + length > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return -1;
    }

    size_t size = blob->buffer_size - length - start;

    for (size_t i = 0; i < size; ++i)
    {
        blob->buffer[start + i] = blob->buffer[start + length + i];
    }

    if (enj_blob__resize(blob, blob->buffer_size - length, err) < 0)
        return -1;

    for (enj_blob_anchor* anchor = blob->anchors; anchor; anchor = anchor->next)
    {
        if (!anchor->valid)
            continue;

        if (anchor->pos > start && anchor->pos < start + length)
        {
            anchor->valid = 0;
            anchor->pos = start;
        }
        else if (anchor->pos >= start + length)
        {
            anchor->pos -= length;
        }
    }

    if (enj_blob__update_cursors(blob, err) < 0)
        return -1;

    return 0;
}

int enj_blob_move(enj_blob* blob, size_t src, size_t dest, size_t length, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!length)
        return 0;

    if (src + length > blob->buffer_size ||
        dest + length > blob->buffer_size)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return -1;
    }

    memmove(blob->buffer + dest, blob->buffer + src, length);

    for (enj_blob_anchor* anchor = blob->anchors; anchor; anchor = anchor->next)
    {
        if (!anchor->valid)
            continue;

        if ((anchor->pos > src || (anchor->is_cursor_end && anchor->pos == src)) && anchor->pos <= src + length)
        {
            if (!anchor->is_cursor_end && anchor->pos == src)
            anchor->pos += (dest - src);
        }
    }

    if (enj_blob__update_cursors(blob, err) < 0)
        return -1;

    return 0;
}

int enj_blob__update_cursors(enj_blob* blob, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_blob_cursor* cursor = blob->cursors; cursor; cursor = cursor->next)
    {
        if (!cursor->valid)
            continue;

        if (!cursor->start->valid || !cursor->end->valid)
        {
            cursor->valid = 0;
            continue;
        }

        size_t length = cursor->end->pos - cursor->start->pos;
        if (cursor->length < length)
        {
            cursor->grown = 1;
            cursor->length = length;
        }
        else if (cursor->length > length)
        {
            cursor->shrank = 1;
            cursor->length = length;
        }
    }

    return 0;
}

int enj_blob__resize(enj_blob* blob, size_t new_size, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!new_size)
    {
        enj_free(blob->buffer);
        blob->buffer = 0;
        blob->chunks = 0;
    }
    else if (new_size == blob->buffer_size)
    {
        return 0;
    }
    else if (new_size < blob->buffer_size)
    {
        while (blob->chunks * blob->chunk_size - new_size >= blob->chunk_size)
        {
            if (enj_blob__shrink(blob, err) < 0)
                return -1;
        }
    }
    else if (new_size > blob->buffer_size)
    {
        while (blob->chunks * blob->chunk_size < new_size)
        {
            if (enj_blob__grow(blob, err) < 0)
                return -1;
        }
    }

    blob->buffer_size = new_size;

    return 0;
}

int enj_blob__grow(enj_blob* blob, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    size_t chunk_start = blob->chunks * blob->chunk_size;

    blob->buffer = enj_realloc(blob->buffer, chunk_start + blob->chunk_size);
    if (!blob->buffer)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return -1;
    }

    memset(blob->buffer + chunk_start, 0, blob->chunk_size);
    ++blob->chunks;

    return 0;
}

int enj_blob__shrink(enj_blob* blob, enj_error** err)
{
    if (!blob)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!blob->chunks)
    {
        enj_error_put(err, ENJ_ERR_BOUNDS);
        return -1;
    }

    --blob->chunks;

    blob->buffer = enj_realloc(blob->buffer, blob->chunks * blob->chunk_size);
    if (!blob->buffer && blob->chunks)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return -1;
    }

    return 0;
}
