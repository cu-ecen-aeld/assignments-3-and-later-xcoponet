/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    if(buffer->out_offs == buffer->in_offs && !buffer->full)
    {
        printf("Empty buffer\n");
        return NULL;
    }

    uint8_t out_idx = buffer->out_offs;
    if(char_offset == 0)
    {
        *entry_offset_byte_rtn = 0;
        return &buffer->entry[buffer->out_offs];
    }

    uint8_t maxread_op = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    while(char_offset >= 0 && maxread_op-- > 0)
    {
        // printf("char_offset: %zu\n", char_offset);
        // printf("buffer->out_offs: %d\n", buffer->out_offs);
        // printf("buffer->in_offs: %d\n", buffer->in_offs);
        // printf("buffer->full: %d\n", buffer->full);
        // printf("buffer->entry[buffer->out_offs].size: %zu\n", buffer->entry[buffer->out_offs].size);
        // printf("buffer->entry[buffer->out_offs].buffptr: %s\n", buffer->entry[buffer->out_offs].buffptr);
        if(buffer->out_offs == buffer->in_offs && !buffer->full)
        {
            buffer->out_offs = out_idx;
            return NULL;
        }

        if(char_offset >= buffer->entry[buffer->out_offs].size)
        {
            char_offset -= buffer->entry[buffer->out_offs].size;
            buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        }
        else
        {
            *entry_offset_byte_rtn = char_offset;
            struct aesd_buffer_entry * res = &buffer->entry[buffer->out_offs];
            buffer->out_offs = out_idx;
            return res;
        }
        
    }
    buffer->out_offs = out_idx;
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    if(!buffer->full)
    {
        buffer->entry[buffer->in_offs] = *add_entry;
        buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        if(buffer->in_offs == buffer->out_offs)
        {
            buffer->full = true;
        }
    }
    else
    {
        buffer->entry[buffer->out_offs] = *add_entry;
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
