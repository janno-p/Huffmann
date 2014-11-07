/**
 * bitstream.h
 *
 * Custom stream library for writing files bit-by-bit
 *
 * @author Janno Põldma
 * @version 01.11.2008 20:00
 */

#ifndef __INCLUDES_BITSTREAM_H__
#define __INCLUDES_BITSTREAM_H__

// Needed for reading/writing integer data
#define ULONG_GET_MASK 	0x80000000
#define ULONG_SET_MASK 	0x00000001
#define ULONG_WIDTH 	32

// Needed for reading/writing character data
#define UCHAR_GET_MASK 	0x80
#define UCHAR_SET_MASK 	0x01
#define UCHAR_WIDTH 	8

#ifndef __UCHAR_DEFINED__
#define __UCHAR_DEFINED__
typedef unsigned char uchar;
#endif

#ifndef __UINT_DEFINED__
#define __UINT_DEFINED__
typedef unsigned int uint;
#endif

// Enumeration type for describing bit values
enum BIT
{
	LOW = 0,
	HIGH = 1,
};

// Enumeration type for bitstream mode
enum BITSTREAMMODE
{
	READ = 0,
	WRITE = 1,
};

// Structure to hold file and written bytes info
typedef struct BITSTREAM
{
	FILE* file; 				// binary file which contains byte data
	enum BITSTREAMMODE mode;	// how this stream is used
	uchar byte_buffer;			// active byte loaded from file
	uint byte_buffer_count;		// how many bits we have already used from byte
} BITSTREAM;

// Creates new bitstream from given file, rewinds the file to read from the
// beginning
BITSTREAM* bs_create(FILE* file_in, enum BITSTREAMMODE mode);

// Releases bitstream which was created by bs_create method
int bs_destroy(BITSTREAM* bs);

// Reads next bit from the stream
int bs_read_bit(BITSTREAM* bs, enum BIT* bit);

// Writes given bit to the stream
int bs_write_bit(BITSTREAM* bs, enum BIT bit);

#endif // __INCLUDES_BITSTREAM_H__
