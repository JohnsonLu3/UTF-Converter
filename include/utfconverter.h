#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include <limits.h>
#include <sys/times.h>
#include <math.h>

#define MAX_BYTES 4
#define SURROGATE_SIZE 4
#define NON_SURROGATE_SIZE 2
#define NO_FD -1
#define OFFSET 2

#define FIRST  0 
#define SECOND 1 
#define THIRD  2 
#define FOURTH 3 

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif

/** The enum for endianness. */
typedef enum {LITTLE, BIG, UTF8} endianness;

/** The struct for a codepoint glyph. */
typedef struct Glyph {
	endianness end;
	bool surrogate;
	unsigned char bytes[MAX_BYTES];
} Glyph;

/** The given filename. */
extern char* filename;

/** The usage statement. */
const char* USAGE[] = {
"Command line utility for converting UTF files to and from UTF-8, UTF-16LE and UTF-16BE.\n\n",
"./utf [-h|--help] -u OUT_ENC | --UTF=OUT_ENC IN_FILE [OUT_FILE]\n\n",
"Option arguments:\n\t",
"-h, --help\t",
"Displays this usage.\n\t",
"-v, -vv\t\t",
"Toggles the verbosity of the program to level 1 or 2.\n\n",
"Mandatory argument:\n\t",
"-u OUT_ENC, --UTF=OUT_ENC\t",
"Sets the output encoding.\n\t\t\t\t\t",
"Valid values for OUT_ENC: 16LE, 16BE\n\n",
"Positional Arguments:\n\t",
"IN_FILE\t\t",
"The file to convert.\n\t",
"[OUT_FILE]\t",
"Output file name. If not present, defaults to stdout.\n"
};

/** Which endianness to convert to. */
extern endianness conversion;

/** Which endianness the source file is in. */
extern endianness source;

void print_verbosity(int level);
	
/**
* Checks if the output file contains a BOM at the start
* if the file is empty then add the BOM
*/
int checkOutBom();

int swapEnd(int byte2swap);
/**
 * A function that swaps the endianness of the bytes of an encoding from
 * LE to BE and vice versa.
 *
 * @param glyph The pointer to the glyph struct to swap.
 * @return Returns a pointer to the glyph that has been swapped.
 */
Glyph* swap_endianness P((Glyph*));

/**
 * Fills in a glyph with the given data in data[2], with the given endianness 
 * by end.
 *
 * @param glyph 	The pointer to the glyph struct to fill in with bytes.
 * @param data[2]	The array of data to fill the glyph struct with.
 * @param end	   	The endianness enum of the glyph.
 * @param fd 		The int pointer to the file descriptor of the input 
 * 			file.
 * @return Returns a pointer to the filled-in glyph.
 */
Glyph* fill_glyph P((Glyph*, unsigned int data[2], endianness, int*));

/**
* A function that converts a UTF-8 glyph to a UTF-16LE or UTF-16BE
* glyph, and returns the result as a pointer to the converted glyph.
*
* @param glyph The UTF-8 glyph to convert.
* @param end The endianness to convert to (UTF-16LE or UTF-16BE).
* @return The converted glyph.
*/
Glyph* convert(Glyph* glyph, endianness end, int fd);

/**
 * Writes the given glyph's contents to stdout.
 *
 * @param glyph The pointer to the glyph struct to write to stdout.
 */
void write_glyph P((Glyph*));

/**
 * Calls getopt() and parses arguments.
 *
 * @param argc The number of arguments.
 * @param argv The arguments as an array of string.
 */
void parse_args P((int, char**));

/**
 * Prints the usage statement.
 */
void print_help P((void));

/**
 * Closes file descriptors and frees list and possibly does other
 * bookkeeping before exiting.
 *
 * @param The fd int of the file the program has opened. Can be given
 * the macro value NO_FD (-1) to signify that we have no open file
 * to close.
 */
void quit_converter P((int));
