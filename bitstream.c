/**
 * bitstream.c
 *
 * Implementation of the custom bitstream library
 *
 * @author Janno Põldma
 * @version 01.11.2008 20:58
 */

#include <stdio.h>
#include <stdlib.h>

#include "bitstream.h"

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE 1
#endif

/**
 * Public methods of the bitstream library
 */

// Creates new bitstream from the file given
// Mode describes how this stream is used (not recommended to mix modes)
BITSTREAM* bs_create(FILE* file_in, enum BITSTREAMMODE mode)
{
	// Try to allocate memory for the stream
	BITSTREAM* bs = (BITSTREAM*)malloc(sizeof(BITSTREAM));
	if (bs == NULL) {
		perror("Could not allocate memory for bitstream (out of memory)");
		return NULL;
	}
	
	// Initialize stream and its buffer
	bs->file = file_in;
	bs->mode = mode;
	bs->byte_buffer = 0;
	bs->byte_buffer_count = 0;
	
	return bs;
}

// Releases the stream object and its allocated memory
int bs_destroy(BITSTREAM* bs)
{
	// In case the stream was opened in write mode, we should flush the buffer
	// to the file (in case it has something in it)
	if ((bs->mode == WRITE) && (bs->byte_buffer_count))
	{
		// Fill extra bits of the buffer with low bits
		bs->byte_buffer <<= (UCHAR_WIDTH - bs->byte_buffer_count);
		// Write the buffer to the file
		if ((fputc(bs->byte_buffer, bs->file) == EOF) && ferror(bs->file)) {
			perror("Error occured when writing the file");
			return FAILURE;
		}
	}
	// Release allocated memory
	free(bs);
	return SUCCESS;
}

// Reads single bit from the stream
int bs_read_bit(BITSTREAM* bs, enum BIT* bit)
{
	int ch;

	// In case our buffer is empty, we must read next byte from the file
	if (!bs->byte_buffer_count)
	{
		// If we're at the end of file, then something must be wrong
		if (feof(bs->file)) {
			fprintf(stderr, "Unexpected end of file!");
			return FAILURE;
		}
		// Read next byte from the file
		ch = fgetc(bs->file);
		if ((ch == EOF) && ferror(bs->file)) {
			perror("Error occured when reading the file");
			return FAILURE;
		}
		bs->byte_buffer = (uchar)ch;
		// Set buffer size to full
		bs->byte_buffer_count = UCHAR_WIDTH;
	}
	// Read next bit value and save it
	*bit = (bs->byte_buffer & UCHAR_GET_MASK) ? HIGH : LOW;
	// Set cursor to the next bit
	bs->byte_buffer <<= 1;
	// Decrease the number of bits in the buffer
	bs->byte_buffer_count--;

	return SUCCESS;
}

// Writes next bit to the stream
int bs_write_bit(BITSTREAM* bs, enum BIT bit)
{
	// Add new bit to the buffer (initially low)
	bs->byte_buffer <<= 1;
	if (bit == HIGH)
	{
		// Bit must be high, set it so
		bs->byte_buffer |= UCHAR_SET_MASK;
	}
	// Increase the number of bits in the buffer
	bs->byte_buffer_count++;
	// In case we have reached the limit of the buffer, write buffer to the file
	if (bs->byte_buffer_count >= UCHAR_WIDTH)
	{
		if ((fputc(bs->byte_buffer, bs->file) == EOF) && ferror(bs->file)) {
			perror("Error occured when writing the file");
			return FAILURE;
		}
		// Reset the buffer
		bs->byte_buffer = 0;
		bs->byte_buffer_count = 0;
	}
	return SUCCESS;
}
