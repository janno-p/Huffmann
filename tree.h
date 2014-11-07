/**
 * tree.h
 *
 * Describes tree structure which contains statistical info about input file
 *
 * @author Janno Põldma
 * @version 02.11.2008 14:46
 */

#ifndef __INCLUDES_TREE_H__
#define __INCLUDES_TREE_H__

#define MAX_CHAR 256

#ifndef __UCHAR_DEFINED__
#define __UCHAR_DEFINED__
typedef unsigned char uchar;
#endif

#ifndef __UINT_DEFINED__
#define __UINT_DEFINED__
typedef unsigned int uint;
#endif

#ifndef __ULONG_DEFINED__
#define __ULONG_DEFINED__
typedef unsigned long ulong;
#endif

// Describes single object in the tree which contains statistical info
typedef struct NODE
{
	uchar ch;				// What character this node represents
	uint freq;				// How frequent characters this node contains
	struct NODE* parent;	// Parent node for this node
	struct NODE* left;		// Left child node (NULL if this node is leaf)
	struct NODE* right;		// Right child node (NULL if this node is leaf)
} NODE;

// Holds information about character nodes
typedef struct TREE
{
	struct NODE* root;					// Node which is the base for the others
	struct NODE* node_list[MAX_CHAR];	// Allows easy access to all nodes 
} TREE;

// Constructs new tree based on file contents
TREE* build_tree(FILE* file_in);

// Releases memory allocated by the tree structure
void release_tree(TREE* tree);

#endif // __INCLUDES_TREE_H__
