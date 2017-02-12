#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>


#define MAX_BYTES 4
#define SURROGATE_SIZE 4
#define NON_SURROGATE_SIZE 2
#define NO_FD -1
#define OFFSET 2
#define E_OFFSET 1

#define FIRST  0
#define SECOND 1
#define THIRD  2
#define FOURTH 3

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif

#if defined(_WIN32) || defined(_WIN64)
        const char* os = "Windows";
#else
#ifdef __linux
        const char* os = "Linux";
#else
        const char* os = "Solaris";
#endif
#endif

#define BLU   "\x1B[34m"
#define GRN   "\x1B[32m"
#define RED  "\x1B[31m"
#define RESET "\x1B[0m"

#define SURROGATE_PAIR 0x10000

/** The enum for endianness. */
typedef enum {LITTLE, BIG, EIGHT} endianness;

/** The struct for a codepoint glyph. */
typedef struct Glyph {
	unsigned char bytes[MAX_BYTES];
	endianness end;
	bool surrogate;
} Glyph;

/** The given filename. */
extern char* filename;

/** The usage statement. */
const char* USAGE[]= {
"\nCommand line utility for converting UTF files to and from UTF-8, UTF-16LE\n",
"and UTF-16BE.\n\n",
"Usage:\n",
"\t./utf [-h | --help | -v | -vv] -u OUT_ENC | --UTF=OUT_ENC IN_FILE [OUT_FILE]\n\n",
"\tOption arguments:\n",
"\t\t-h, --help\t\tDisplays this usage\n",
"\t\t-v, -vv\t\t\tToggles the verbosity of the program to level 1 or 2.\n\n",
"\tMandatory argument:\n",
"\t\t-u OUT_ENC, --UTF=OUT_ENC\tSets the output encoding.\n",
"\t\t\t\t\t\tValid values for OUT_ENC: 8, 16LE, 16BE\n\n",
"\tPositional Arguments:\n",
"\t\tIN_FILE\t\t\tThe file to convert.\n",
"\t\t[OUT_FILE]\t\tOutput file name. If not present, defaults to stdout.\n",
};

/** Which endianness to convert to. */
extern endianness conversion;

/** Which endianness the source file is in. */
extern endianness source;

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
Glyph* fill_glyph P((Glyph* glyph, unsigned char bytes[], endianness, int*));

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
 * Prints the verbose.
 */
void print_verbose (int verbose);

/**
 * Closes file descriptors and frees list and possibly does other
 * bookkeeping before exiting.
 *
 * @param The fd int of the file the program has opened. Can be given
 * the macro value NO_FD (-1) to signify that we have no open file
 * to close.
 */
void quit_converter P((int));


Glyph* read_utf_8(int *fd, Glyph* glyph);

Glyph* convert(Glyph* glyph, endianness end);
