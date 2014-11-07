/**
 * compression.c
 *
 * Implementation of the compression library
 *
 * @author Janno Põldma
 * @version 02.11.2008 13:27
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "compression.h"
#include "tree.h"

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE 1
#endif

/**
 * Definitions for the private methods of the library
 */

// Reads the size of the original file from the stream
int get_length(BITSTREAM* bs, ulong* size);

// Reads the tree structure from the stream
int get_tree(BITSTREAM* bs, TREE** tree);

// Reads the node and its subnodes from the stream
int get_node(BITSTREAM* bs, TREE* tree, NODE** node);

// Reads the character from the stream
int get_char(BITSTREAM* bs, NODE* node, uchar* ch);

// Writes specified node information to the stream
int put_node(BITSTREAM* bs, NODE* node);

// Writes specified character to the stream
int put_char(BITSTREAM* bs, uchar ch);

// Writes the size of the original file to the stream
int put_length(BITSTREAM* bs, ulong size);

// Writes the encoding tree to the stream
int put_tree(BITSTREAM* bs, NODE* node);

/**
 * Implementation of the public library methods
 */

// Encodes contents of the input file and writes result to the output file
int encode(FILE* file_in, FILE* file_out)
{
	long s;
	ulong size;
	TREE* tree;

	// Open new stream for writing
	BITSTREAM* bs = bs_create(file_out, WRITE);
	if (bs == NULL) {
		return FAILURE;
	}
	
	// Find out the size of the original file
	if (fseek(file_in, 0, SEEK_END)) {
		perror("Could not seek end of input file");
		bs_destroy(bs);
		return FAILURE;
	}
	s = ftell(file_in);
	if (s == 1L) {
		perror("Could not tell cursor location in the input file");
		bs_destroy(bs);
		return FAILURE;
	}
	size = (ulong)s;
	
	// Write size of the file to the stream
	if (put_length(bs, size) == FAILURE) {
		bs_destroy(bs);
		return FAILURE;
	}

	// Go to the beginning of the source file and build Huffmann tree based on
	// its contents
	rewind(file_in);	
	tree = build_tree(file_in);
	if (tree == NULL) {
		bs_destroy(bs);
		return FAILURE;
	}
	
	// Write the tree structure to the file
	if (put_tree(bs, tree->root) == FAILURE) {
		bs_destroy(bs);
		release_tree(tree);
		return FAILURE;
	}
	
	// Encode the contents of the input file and write them to the target file
	rewind(file_in);
	while (!feof(file_in)) {
		// Read next character
		int ch = fgetc(file_in);
		if ((ch == EOF) && ferror(file_in)) {
			perror("Error occured when reading the file");
			return FAILURE;
		}
		// Encode starting from the character node to the top of the tree
		if (put_node(bs, tree->node_list[ch]) == FAILURE) {
			release_tree(tree);
			bs_destroy(bs);
			return FAILURE;
		}
	}
	
	// Release resources allocated by the tree and stream
	release_tree(tree);
	bs_destroy(bs);
	
	return SUCCESS;
}

// Decodes the contents of the source file and writes result to the output file
int decode(FILE* file_in, FILE* file_out)
{
	ulong size;
	TREE* tree;
	ulong i;

	// Opens the bitstream for the input file
	BITSTREAM* bs = bs_create(file_in, READ);
	if (bs == NULL) {
		return FAILURE;
	}
	
	// Tries to extract original file size from the stream
	if (get_length(bs, &size) == FAILURE) {
		bs_destroy(bs);
		return FAILURE;
	}
	
	// Tries to extract encoding tree from the stream
	if (get_tree(bs, &tree) == FAILURE) {
		bs_destroy(bs);
		return FAILURE;
	}
	
	// Tries to decode rest of the file and writes to the target file
	for (i = 0; i < size; i++) {
		uchar ch;
		if (get_char(bs, tree->root, &ch) == FAILURE) {
			bs_destroy(bs);
			return FAILURE;
		}
		if ((fputc(ch, file_out) == EOF) && ferror(file_out)) {
			perror("Error occured when writing the file");
			bs_destroy(bs);
			return FAILURE;
		}
		fprintf(stderr, "%c", ch);
	}
	
	// Releases allocated resources
	release_tree(tree);
	bs_destroy(bs);
	
	return SUCCESS;
}

/**
 * Private methods of the library
 */

// Get the length of the original file
int get_length(BITSTREAM* bs, ulong* size)
{
	enum BIT bit;
	// Read the size of the file bit-by-bit
	int i;
	for (i = 0; i < ULONG_WIDTH; i++) {
		// Move cursor and read next bit
		(*size) <<= 1;
		if (bs_read_bit(bs, &bit) == FAILURE) {
			return FAILURE;
		}
		if (bit == HIGH) {
			(*size) |= ULONG_SET_MASK;
		}
	}
	fprintf(stderr, "%d\n", *size);
	return SUCCESS;
}

// Get the tree structure from the file
int get_tree(BITSTREAM* bs, TREE** tree)
{
	// Allocate memory
	*tree = (TREE*)malloc(sizeof(TREE));
	if (*tree == NULL) {
		perror("Could not allocate memory for tree structure (out of memory)");
		return FAILURE;
	}
	// Reset node list
	memset((*tree)->node_list, 0, MAX_CHAR * sizeof(NODE*));
	// Read node relations from the stream
	if (get_node(bs, *tree, &((*tree)->root)) == FAILURE) {
		free(*tree);
		return FAILURE;
	}
	return SUCCESS;
}

// Read next node from the current position at the stream
int get_node(BITSTREAM* bs, TREE* tree, NODE** node)
{
	enum BIT bit;

	// Allocate memory for the node
	(*node) = (NODE*)malloc(sizeof(NODE));
	if (*node == NULL) {
		perror("Could not allocate memory for node (out of memory)");
		return FAILURE;
	}
	// Reset the structure
	memset(*node, 0, sizeof(NODE));
	
	// Read next bit from the stream
	if (bs_read_bit(bs, &bit) == FAILURE) {
		free(*node);
		return FAILURE;
	}
	
	// If it is high bit then we're at the branch node, so read the leafs also
	if (bit == HIGH) {
		if ((get_node(bs, tree, &((*node)->left)) == FAILURE) || (get_node(bs, tree, &((*node)->right)) == FAILURE)) {
			free(*node);
			return FAILURE;
		}
	} else {
		// This must be leaf node, so read the character it represents
		int i;
		for (i = 0; i < UCHAR_WIDTH; i++) {
			(*node)->ch <<= 1;
			if (bs_read_bit(bs, &bit) == FAILURE) {
				free(*node);
				return FAILURE;
			}
			if (bit == HIGH) {
				(*node)->ch |= UCHAR_SET_MASK;
			}
		}
		// Check if this character is already loaded
		if (tree->node_list[(*node)->ch] != NULL) {
			// Cannot read same character twice
			fprintf(stderr, "Archive is corrupted!\n");
			free(*node);
			return FAILURE;
		} else {
			// Bind character to the node
			tree->node_list[(*node)->ch] = *node;
		}
	}
	return SUCCESS;	
}

// Decodes the character by climbing on the huffmann tree
int get_char(BITSTREAM* bs, NODE* node, uchar* ch)
{
	enum BIT bit;
	NODE* next;

	if (bs_read_bit(bs, &bit) == FAILURE) {
		return FAILURE;
	}

	// Get the next node for decoding
	next = (bit == HIGH) ? node->right : node->left;
	if (next == NULL) {
		// Should not reach here unless ...
		fprintf(stderr, "Archive is corrupted!\n");
		return FAILURE;
	}

	// If child nodes are empty then we are on the leaf, so return the character
	if (next->left == NULL) {
		*ch = next->ch;
		return SUCCESS;
	}
	
	// We have to search the character from child node
	return get_char(bs, next, ch);
}

// Encodes current node and all of its parent nodes info to the target file_in
// as encoded character
int put_node(BITSTREAM* bs, NODE* node)
{
	// First write parent bit
	// Encoding writes from bottom to the top
	// Decoding reads from top to the bottom
	if (node->parent != NULL) {
		if (node->parent->parent != NULL) {
			if (put_node(bs, node->parent) == FAILURE) {
				return FAILURE;
			}
		}
		return bs_write_bit(bs, (node->parent->left == node) ? LOW : HIGH);
	}
	return SUCCESS;
}

// Writes concrete character to the file
int put_char(BITSTREAM* bs, uchar ch)
{
	int i;
	for (i = 0; i < UCHAR_WIDTH; i++) {
		// Write the highest bit to the file 
		if (bs_write_bit(bs, (ch & UCHAR_GET_MASK) ? HIGH : LOW) == FAILURE) {
			return FAILURE;
		}
		// Move cursor to the next bit
		ch <<= 1;
	}
	return SUCCESS;
}

// Writes the size of the original file to the stream
int put_length(BITSTREAM* bs, ulong size)
{
	int i;
	for (i = 0; i < ULONG_WIDTH; i++) {
		// Write the highest bit of the long variable
		if (bs_write_bit(bs, (size & ULONG_GET_MASK) ? HIGH : LOW) == FAILURE) {
			return FAILURE;
		}
		// Shift the cursor to the next bit
		size <<= 1;
	}
	return SUCCESS;
}

// Writes the tree to the file starting from the root node
int put_tree(BITSTREAM* bs, NODE* node)
{
	static int tab = 0;
	int i;
	for (i = 0; i < tab; i++)
		fprintf(stderr, " ");
	// If node is leaf, then write starting low bit and corresponding character
	if ((node->left == NULL) && (node->right == NULL)) {
		fprintf(stderr, "%c\n", node->ch);
		return (bs_write_bit(bs, LOW) == SUCCESS) ? put_char(bs, node->ch) : FAILURE; 
	}
	fprintf(stderr, "@\n");
	tab++;
	// If node is branch write high bit and both child nodes
	if ((bs_write_bit(bs, HIGH) == FAILURE) || (put_tree(bs, node->left) == FAILURE) || (put_tree(bs, node->right) == FAILURE)) {
		return FAILURE;
	}
	tab--;
	return SUCCESS;
}
