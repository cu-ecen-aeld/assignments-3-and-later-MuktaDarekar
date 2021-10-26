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
#include <linux/slab.h>
#define FREE	kfree
#else
#include <string.h>
#include <stdlib.h>
#define FREE	free
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
    /**
    * TODO: implement per description
    */
    //get copy of out offset
    uint8_t temp_out_offs = buffer->out_offs;
    size_t size = buffer->entry[temp_out_offs].size;
    size_t prev_size = 0;

    //buffer empty check
    if((buffer->full == false) && (buffer->in_offs == buffer->out_offs))
        return NULL;

    //get aesd_buffer_entry pointer as per char_offset
    while(1)
    {
        //if char_offset is greater than size of current entry then move to next one
        if(char_offset >= size)
        {	
            prev_size = size;

            temp_out_offs++;
            if(temp_out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
                temp_out_offs = 0;

            size = size + buffer->entry[temp_out_offs].size;
        }
        //Get a pointer specifying a location as per char_offset
        else
        {
            *entry_offset_byte_rtn = char_offset - prev_size;
            return (&(buffer->entry[temp_out_offs]));	
        }
        //stop looking if reference pointer reaches to in offset of buffer
        if(temp_out_offs == buffer->in_offs)
            break;
    }

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char * aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
    
	const char *scrap_entry = NULL;

    //check for buffer full and out offset limit
    if(buffer->full)
    {
    	//entry to be scraped
    	scrap_entry = buffer->entry[buffer->out_offs].buffptr;
    	
    	//Load new entry at in offset
		buffer->entry[buffer->in_offs] = *add_entry;

		//check in offset limit
		buffer->in_offs++;
		if(buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
		    buffer->in_offs = 0;
		    
        buffer->out_offs++;
        if(buffer->out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
            buffer->out_offs = 0;
    }
    else
    {
    	//Load new entry at in offset
		buffer->entry[buffer->in_offs] = *add_entry;

		//check in offset limit
		buffer->in_offs++;
		if(buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
		    buffer->in_offs = 0;
		    
        if(buffer->out_offs == buffer->in_offs)
            buffer->full = true;
        else
            buffer->full = false;
    }
    
    return scrap_entry;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

/**
* free the circular buffer described by @param buffer to delete each entry
*/
void aesd_circular_buffer_delete(struct aesd_circular_buffer *buffer)
{
	uint8_t index;
	struct aesd_buffer_entry *entry;
	AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index) 
	{
		if(entry == NULL)
			continue;
			
		FREE((void *)entry->buffptr);
		/*
		#ifdef __KERNEL__
			kfree(entry->buffptr);
		#else
			free(entry->buffptr);
		#endif
		*/
	}
}




