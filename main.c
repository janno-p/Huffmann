/**
 * main.c
 *
 * Main entry point of the application
 *
 * @author Janno Põldma
 * @version 02.11.2008 00:05
 */

#include <stdio.h>
#include <string.h>

#include "compression.h"

// Possible to add other options later
enum OPTIONS
{
	DECODE = 0x01,
};

// Reads specified options from the command line argument
int read_options(char* args);

// Main entry point of the application
int main(int argc, char** argv)
{
	// Initialize options to none
	int options = 0;

	// Go through all extra command line parameters and read options
	int i;
	for (i = 1; i < argc; i++) {
		options |= read_options(argv[i]);
	}
	
	// If decoding option was specified, then decode from source to target
	if (options & DECODE) {
		return decode(stdin, stdout);
	}
	
	// Otherwise encode from source to target
	return encode(stdin, stdout);
}

// Reads options from the specified string
int read_options(char* args)
{
	// Initial state
	int options = 0;
	
	// Go through each character in the string
	int length = strlen(args);
	if ((length > 0) && (args[0] == '-')) {
		// Find out letters of our interest
		int i;
		for (i = 0; i < length; i++) {
			switch (args[i]) {
				case 'd': options |= DECODE; break;
			}
		}
	}
	
	return options;
}
