/**
 * compression.h
 *
 * Methods for compressing given file with huffmann algorithm
 *
 * @author Janno Põldma
 * @version 02.11.2008 13:01
 */

#ifndef __INCLUDES_COMPRESSION_H__
#define __INCLUDES_COMPRESSION_H__

// Encodes entire file_in contents and writes output to the file_out
// Returns error code
int encode(FILE* file_in, FILE* file_out);

// Decodes contents of file_in and writes output to the file_out
// Returns error code
int decode(FILE* file_in, FILE* file_out);

#endif // __INCLUDES_COMPRESSION_H__
