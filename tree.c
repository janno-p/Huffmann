/**
 * tree.c
 *
 * Implementation of the tree constructing algorithm
 *
 * @author Janno Põldma
 * @version 02.11.2008 14:57
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE 1
#endif

// Type for defining how many times each character occurs in compressed file
typedef uint FREQTABLE[MAX_CHAR];

/*
 * Definitions for all functions this library is using
 */
 
// Calculates frequencies of all characters in file we are compressing
int calc_freq_table(FILE* file_in, FREQTABLE freq_table);

// Initializes list of nodes in the tree, returns not 0 on success
int init_node_list(TREE* tree, FREQTABLE freq_table);

// Function for sorting nodes by the frequency of the character it contains
int compare_nodes(const void* n1, const void* n2);

// Adds new node to the node list depending on its value
void add_node(NODE** nodes, uint count, NODE* node);

// Releases all memory allocated by this node and all its subnodes
void release_node(NODE* node);

/*
 * Implementation of all public library methods
 */

// Builds new character/huffmann tree based on information from the source file
TREE* build_tree(FILE* file_in)
{
	TREE* tree;
	NODE* sorted_nodes[MAX_CHAR];
	uint node_count;
	NODE* smallest;
	NODE* small;
	NODE* node;
	uint i;

	// Calculate character frequencies
	FREQTABLE freq_table;
	if (calc_freq_table(file_in, freq_table) == FAILURE) {
		return NULL;
	}
	
	// Allocate memory for the tree
	tree = (TREE*)malloc(sizeof(TREE));
	if (tree == NULL) {
		perror("Failed to allocate memory for frequency-tree (out of memory)");
		return NULL;
	}
	tree->root = NULL;
	
	// Create list of leaf nodes
	if (init_node_list(tree, freq_table) == FAILURE) {
		// Release the memory allocated by the tree
		release_tree(tree);
		return NULL;
	}
	
	// Sort the nodes depending on character frequency in file
	memcpy(sorted_nodes, tree->node_list, MAX_CHAR * sizeof(NODE*));
	qsort(sorted_nodes, MAX_CHAR, sizeof(NODE*), compare_nodes);
	
	// Find out how many nodes are in the list
	// (maximum is the number of different characters [256])
	node_count = 0;
	while ((sorted_nodes[node_count] != NULL) && (node_count < MAX_CHAR)) {
		node_count++;
	}
	
	// Arrange nodes to the tree, creating branches and leaf according to
	// Huffmann algorithm
	// Loops through all the nodes by taking least frequent character nodes
	// adding them together and inserting to the list
	// Each iteration decreases elements in temporary node list by one element
	for ( ; node_count > 1; node_count--) {
		// Two lowest frequency characters in list
		smallest = sorted_nodes[node_count - 1];
		sorted_nodes[node_count - 1] = NULL;
		small = sorted_nodes[node_count - 2];
		sorted_nodes[node_count - 2] = NULL;
		
		// Allocate space for branch node
		node = (NODE*)malloc(sizeof(NODE));
		if (node == NULL) {
			perror("Failed to allocate memory for branch node (out of memory)");
			// Release nodes we just extracted from the list
			release_node(smallest);
			release_node(small);
			// Release all nodes in the list
			for (i = 0; i < node_count - 2; i++) {
				if (sorted_nodes[i] != NULL) {
					release_node(sorted_nodes[i]);
				}
			}
			// Release memory allocated by the tree structure
			release_tree(tree);
			return NULL;
		}
		
		// Set initial values for the branch node
		node->ch = 0;
		node->freq = smallest->freq + small->freq;
		node->parent = NULL;
		
		// Set leafs for this node (including parent info for the leafs)
		node->left = smallest;
		node->left->parent = node;
		node->right = small;
		node->right->parent = node;
		
		// Insert node to the nodes list at proper position
		// (ordered by frequency)
		add_node(sorted_nodes, node_count - 2, node);
	}
	
	// Now there should be exactly one element left which is root node to all
	// of other nodes
	tree->root = sorted_nodes[0];
	
	return tree;
}

// Releases memory allocated by the tree structure
void release_tree(TREE* tree)
{
	// If tree already has a root node, then release it and all its child nodes
	if (tree->root != NULL) {
		release_node(tree->root);
	}
	// Free memory allocated by the tree itself
	free(tree);
}

/**
 * Private methods for the library
 */

// Releases single node and all its childs
void release_node(NODE* node)
{
	// If node has left child, then release it and all its child nodes
	if (node->left) {
		release_node(node->left);
	}
	// If node has right child, then release it and all its child nodes
	if (node->right) {
		release_node(node->right);
	}
	// Free the memory allocated by the node itself
	free(node);
}

// Calculate character frequencies by the contents of the input file
int calc_freq_table(FILE* file_in, FREQTABLE freq_table)
{
	// Set all current values in the table 0
	memset(freq_table, 0, sizeof(FREQTABLE));
	// Go through the file and calculate each character count in the file
	while (!feof(file_in)) {
		int ch = fgetc(file_in);
		if ((ch == EOF) && ferror(file_in)) {
			perror("Failed to read from input file");
			return FAILURE;
		}
		freq_table[ch]++;
	}
	return SUCCESS;
}

// Initialize all leaf nodes for the tree (nodes that contain character info)
int init_node_list(TREE* tree, FREQTABLE freq_table)
{
	uint i;
	uint j;

	// Clear the table (so it contains only NULL pointers)
	memset(tree->node_list, 0, MAX_CHAR * sizeof(NODE*));

	// Create node object for each character,
	// which occurs at least once in the file 
	for (i = 0; i < MAX_CHAR; i++) {
		if (freq_table[i] > 0) {
			// Allocate memory for the new node
			NODE* node = (NODE*)malloc(sizeof(NODE));
			if (node == NULL) {
				break;
			}
			memset(node, 0, sizeof(NODE));
			node->ch = (uchar)i;
			node->freq = freq_table[i];
			tree->node_list[i] = node;		
		}
	}
	
	if (i >= MAX_CHAR)
		return SUCCESS;
	
	perror("Failed to allocate memory for leaf node (out of memory)");

	// Release nodes we have already allocated
	for (j = 0; j < i; j++) {
		release_node(tree->node_list[j]);
	}
	
	// Return error code
	return FAILURE;
}

// Compares nodes for sorting by their character frequencies
int compare_nodes(const void* n1, const void* n2)
{
	// Dereference nodes we are comparing
	NODE* node1 = *((NODE**)n1);
	NODE* node2 = *((NODE**)n2);
	
	// In case we have node which is NULL then it should be at the bottom of the
	// list, so its value is maximum negative value
	// Otherwise it equals to the frequency of the character
	uint v1 = (node1 == NULL) ? -MAX_CHAR : node1->freq;
	uint v2 = (node2 == NULL) ? -MAX_CHAR : node2->freq;
	
	// We want to order nodes so that the bigger frequencies are at the top
	return v2 - v1;
}

// Inserts new node to the nodes list
// (inserts it at the position which corresponds to the frequency it holds)
void add_node(NODE** nodes, uint count, NODE* node)
{
	// Start from the bottom of the list to find out the location of the node
	for ( ; count > 0; count--) {
		// Check if current node is bigger than the node we are adding
		if (nodes[count - 1]->freq >= node->freq) {
			// Found the correct position of the node, so we're done here
			break;
		}
		// Move current node one position towards the end of the list
		nodes[count] = nodes[count - 1];
	}
	// Add the node to the list
	nodes[count] = node;
}
