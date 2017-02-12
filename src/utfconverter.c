#include "utfconverter.h"
#include <math.h>

char* filename;
char* out_file = NULL;
int out_fd = STDOUT_FILENO;
endianness source, conversion;
char *s_enc = NULL;
char *o_enc = NULL;
int verbose = 0;
double total_write_sys = 0.0;
double total_write_user = 0.0;
double total_write_real = 0.0;
double total_read_sys = 0.0;
double total_read_user = 0.0;
double total_read_real = 0.0;
double total_conv_sys = 0.0;
double total_conv_user = 0.0;
double total_conv_real = 0.0;
time_t conv_user_start, conv_user_end, conv_sys_start, conv_sys_end;
double total_glyphs = 0;
double total_surrogate = 0;
double total_ascii = 0;


/*------------------------------ MAIN ------------------------------------*/
int main(int argc, char** argv)
{
	unsigned char buf[4]; 
	int rv = 0; 
	Glyph* glyph = malloc(sizeof(Glyph));
	int fd = -1;
	void* memset_return;
	time_t write_real_start, write_real_end, read_real_start, read_real_end, conv_real_start, conv_real_end;
	time_t read_user_start, read_user_end, read_sys_start, read_sys_end;

	/* parse args */
	parse_args(argc, argv);

	/* Open the input file to read in and output file*/
	if(out_file != NULL)
	{
		if ((out_fd = open(out_file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0)
		{
			fprintf(stderr, "Failed to open the file %s\n", out_file);
			quit_converter(NO_FD);
		}
	}
	
	read_real_start = time(NULL);
	if((fd = open(filename, O_RDONLY)) < 0)
	{
		fprintf(stderr,"Error opening the file: %s\n", filename);
		quit_converter(NO_FD);
	}
	else
	{	
		/* Handle BOM bytes */
		read_user_start = time(NULL);

		if((rv = read(fd, &buf[0], 1) == 1) && (rv = read(fd, &buf[1], 1) == 1))
		{ 
			read_sys_start = time(NULL);
			if(buf[0] == 0xff && buf[1] == 0xfe)
			{
				/*file is big endian*/
				source = LITTLE; 
				s_enc = "UTF-16LE";
			} 
			else if(buf[0] == 0xfe && buf[1] == 0xff)
			{
				/*file is little endian*/
				source = BIG;
				s_enc = "UTF-16BE";
			}
			else if(buf[0] == 0xef && buf[1] == 0xbb)
			{
				if((rv = read(fd, &buf[2], 1) == 1))
				{
					if(buf[2] == 0xbf)
					{
						/* file is utf8 */
						source = EIGHT;
						s_enc = "UTF-8";
					}
				}
			} 
			else 
			{
				/*file has no BOM*/
				free(&glyph->bytes); 
				fprintf(stderr, "File has no BOM.\n");
				quit_converter(NO_FD); 
			}

			memset_return = memset(glyph, 0, sizeof(Glyph));
			/* Memory write failed, recover from it: */
			if(memset_return == NULL)
			{
				memset(glyph, 0, sizeof(Glyph));
			}
			read_sys_end = time(NULL);
			total_read_sys += (read_sys_end - read_sys_start);
		}
		if(conversion == LITTLE)
		{
			unsigned char little_FE = 0xFE;
			unsigned char little_FF = 0xFF;
			write(out_fd, &little_FF, 1);
			write(out_fd, &little_FE, 1);
		}
		else if(conversion == BIG)
		{
			unsigned char big_FE = 0xFE;
			unsigned char big_FF = 0xFF;
			write(out_fd, &big_FE, 1);
			write(out_fd, &big_FF, 1);
		}
		else if(conversion == EIGHT)
		{
			unsigned char eight_EF = 0xEF;
			unsigned char eight_BB = 0xBB;
			unsigned char eight_BF = 0xBF;
			write(out_fd, &eight_EF, 1);
			write(out_fd, &eight_BB, 1);
			write(out_fd, &eight_BF, 1);
		}
		/* Now deal with the rest of the bytes.*/
		if((source == LITTLE && conversion == BIG) || (source == BIG && conversion == LITTLE) || (source == LITTLE && conversion == LITTLE) || (source == BIG && conversion == BIG))
		{
			while((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1)
			{
				void* memset_return;

				read_sys_start = time(NULL);

				if(source == conversion)
				{
					write_real_start = time(NULL);
					write_glyph(fill_glyph(glyph, buf, source, &fd));
					write_real_end = time(NULL);
					total_write_real += (write_real_end - write_real_start);
					total_glyphs += 1;
				}
				else
				{
					write_real_start = time(NULL);
					conv_real_start = time(NULL);
					write_glyph(swap_endianness(fill_glyph(glyph, buf, source, &fd)));
					conv_real_end = time(NULL);
					write_real_end = time(NULL);
					total_write_real += (write_real_end - write_real_start);
					total_conv_real += (conv_real_end - conv_real_start);
					total_glyphs += 1;
				}

				memset_return = memset(glyph, 0, sizeof(Glyph));
			        /* Memory write failed, recover from it: */
			        if(memset_return == NULL)
			        {
				        memset(glyph, 0, sizeof(Glyph));
			        }
			    read_sys_end = time(NULL);
			    total_read_sys += (read_sys_end - read_sys_start);
			}
			read_user_end = time(NULL);
			total_read_user = (read_user_end - read_user_start);
		}
		else if((source == EIGHT && conversion == LITTLE) || (source == EIGHT && conversion == BIG))
		{
			while((rv = read(fd, &buf[0], 1)) == 1)
			{
				memset(glyph, 0, sizeof(Glyph));
				glyph->bytes[0] = buf[0];
				write_glyph(convert(read_utf_8(&fd, glyph), conversion));
			}
		}
	}
	read_real_end = time(NULL);
	total_read_real = (read_real_end - read_real_start);

	print_verbose(verbose);
	quit_converter(NO_FD);
	return EXIT_SUCCESS;
}

/*--------------------------- CONVERT UTF8 ------------------------------*/
Glyph* convert(Glyph* glyph, endianness end)
{
	unsigned int cp = 0;

	if((glyph->bytes[0] & 0x80) != 0x80)
	{
		total_ascii++;
		cp = glyph->bytes[0];
	}
	else if((glyph->bytes[0] & 0xE0) == 0xC0)
	{
		cp = (glyph->bytes[0] & 0x1F) << 6;
		cp = cp + (glyph->bytes[1] & 0x3F);
	}
	else if((glyph->bytes[0] & 0xF0) == 0xE0)
	{
		cp = (glyph->bytes[0] & 0x0F) << 6;
		cp = (cp + (glyph->bytes[1] & 0x3F)) << 6;
		cp = (cp + (glyph->bytes[2] & 0x3F));
	}
	else if((glyph->bytes[0] & 0xF8) == 0xF0)
	{
		cp = (glyph->bytes[0] & 0x07) << 6;
		cp = (cp + (glyph->bytes[1] & 0x3F)) << 6;
		cp = (cp + (glyph->bytes[2] & 0x3F)) << 6;
		cp = (cp + (glyph->bytes[3] & 0x3F));
	}

	if(cp >= 0x10000)
	{
		unsigned int cp_prime = cp - 0x10000;
		unsigned int cp_high = cp_prime >> 10;
		unsigned int cp_low = cp_prime & 0x3FF;

		unsigned int w1 = cp_high + 0xD800;
		unsigned int w2 = cp_low + 0xDC00;

		glyph->bytes[0] = (w1 >> 8);
		glyph->bytes[1] = (w1 & 0x00FF);
		glyph->bytes[2] = (w2 >> 8);
		glyph->bytes[3] = (w2 & 0x00FF);
		glyph->surrogate = true;
		total_surrogate++;
	}
	else
	{
		glyph->bytes[0] = (cp >> 8);
		glyph->bytes[1] = (cp & 0xFF);
		glyph->surrogate = false;
	}
	if(end == LITTLE)
	{
		glyph = swap_endianness(glyph);
	}
	glyph->end = end;
	return glyph;
}

/*------------------------------ READ UTF8 -------------------------------*/
Glyph* read_utf_8(int *fd, Glyph* glyph)
{
	if((glyph->bytes[0] & 0x80) != 0x80)
	{
		total_glyphs++;
		return glyph;		
	}

	else if((glyph->bytes[0] & 0xE0) == 0xC0)
	{
		unsigned char c;

		if(read(*fd, &c, 1) == 1)
		{
			if((c & 0xC0) == 0x80)
			{
				glyph->bytes[1] = c;
				total_glyphs++;
				return glyph;
			}	
		} 
		else
		{
			quit_converter(*fd);
		}
	}
	else if((glyph->bytes[0] & 0xF0) == 0xE0)
	{
		unsigned char c;
		if(read(*fd, &c, 1) == 1)
		{
			if((c & 0xC0) == 0x80)
			{
				glyph->bytes[1] = c;
			}

			if(read(*fd, &c, 1) == 1)
			{
				if((c & 0xC0) == 0x80)
				{
					glyph->bytes[2] = c;
					total_glyphs++;
					return glyph;
				}
			}
			else
			{
				quit_converter(*fd);
			}
		}
		else
		{
			quit_converter(*fd);
		}
	}
	else if((glyph->bytes[0] & 0xF8) == 0xF0)
	{
		unsigned char c;

		if(read(*fd, &c, 1) == 1)
		{
			if((c & 0xC0) == 0x08)
			{
				glyph->bytes[1] = c;
			}

			if(read(*fd, &c, 1) == 1)
			{
				if((c & 0xC0) == 0x80)
				{
					glyph->bytes[2] = c;
				}

				if(read(*fd, &c, 1) == 1)
				{
					if((c & 0xC0) == 0x80)
					{
						glyph->bytes[3] = c;
						total_glyphs++;
						return glyph;
					}
				}
				else
				{
					quit_converter(*fd);
				}
			}
			else
			{
				quit_converter(*fd);
			}
		}
		else
		{
			quit_converter(*fd);
		}
	}
	quit_converter(*fd);
	return glyph;
}
/*------------------------------ SWAP ------------------------------------*/
Glyph* swap_endianness(Glyph *glyph)
{
	unsigned char c = glyph->bytes[0];

	conv_user_start = time(NULL);
	conv_sys_start = time(NULL);
		/* Use XOR to be more efficient with how we swap values. */

 	glyph->bytes[0] = glyph->bytes[1];
 	glyph->bytes[1] = c;
 	if(glyph->surrogate)
 	{  
 		/* If a surrogate pair, swap the next two bytes. */
 		unsigned char d = glyph->bytes[2];

 		glyph->bytes[2] = glyph->bytes[3];
 		glyph->bytes[3] = d;
 	}
 	glyph->end = conversion;
 	conv_user_end = time(NULL);
	conv_sys_end = time(NULL);
	total_conv_user += (conv_user_end - conv_user_start);
	total_conv_sys += (conv_sys_end - conv_sys_start);
 	return glyph;
 }

/*------------------------------ FILL ------------------------------------*/
Glyph* fill_glyph(Glyph* glyph, unsigned char bytes[2], endianness end, int *fd) 
{
	// unsigned int bits;

	glyph->bytes[0] = bytes[0];
 	glyph->bytes[1] = bytes[1];

 	if((glyph->bytes[0] & 0x80) != 0x80)
 	{
 		glyph->end = end;
 		return glyph;
 	}
 	else if((glyph->bytes[0] & 0xE0) == 0xC0)
 	{
 		glyph->end = end;
 		return glyph;
 	}
 	else if((glyph->bytes[0] & 0xF0) == 0xE0)
 	{
 		if(read(*fd, &bytes[0], 1) == 1)
 		{
 			glyph->bytes[3] = bytes[0];
 			glyph->end = end;
 			return glyph;
 		}
 		else
 		{
 			fprintf(stderr,"Error reading.\n");
 		}
 	}
 	else if((glyph->bytes[0] & 0xF8) == 0xF0)
 	{
 		if(read(*fd, &bytes[0], 1) == 1 && read(*fd, &bytes[1], 1) == 1)
 		{
 			glyph->bytes[2] = bytes[0];
 			glyph->bytes[3] = bytes[1];
 			glyph->end = end;
 			return glyph;
 		}
 		else
 		{
 			fprintf(stderr,"Error reading.\n");
 		}
 	}

 	// bits = (bytes[0] + (bytes[1] << 8));
 	// /* Check high surrogate pair using its special value range.*/
 	// if(bits > 0xD800 && bits < 0xDBFF)
 	// { 
 	// 	if(read(*fd, &bytes[0], 1) == 1 && read(*fd, (&bytes[1]), 1) == 1)
 	// 	{
 	// 		bits = 0; 
 	// 		bits = (bytes[0] + (bytes[1] << 8));
 	// 		if(bits > 0xDC00 && bits < 0xDFFF)
 	// 		{ 
 	// 			glyph->surrogate = false; 
 	// 		} 
 	// 		else 
 	// 		{
 	// 			lseek(*fd, - OFFSET, SEEK_CUR); 
 	// 			glyph->surrogate = true;
 	// 			glyph->bytes[2] = bytes[0];
 	// 			glyph->bytes[3] = bytes[1];
 	// 			total_surrogate++;
 	// 		}
 	// 	}
 	// }
 
 	return glyph;
 }

/*------------------------------ WRITE ------------------------------------*/
void write_glyph(Glyph* glyph) 
{
	time_t write_user_start, write_user_end, write_sys_start, write_sys_end;

	write_user_start = time(NULL);
 	if(glyph->surrogate)
 	{
 		write_sys_start = time(NULL);
 		write(out_fd, glyph->bytes, SURROGATE_SIZE);
 		write_sys_end = time(NULL);
 		total_write_sys += (write_sys_end - write_sys_start);
 	} 
 	else 
 	{	write_sys_start = time(NULL);
 		write(out_fd, glyph->bytes, NON_SURROGATE_SIZE);
 		write_sys_end = time(NULL);
 		total_write_sys += (write_sys_end - write_sys_start);
 	}
 	write_user_end = time(NULL);
 	total_write_user += (write_user_end - write_user_start);
 }

/*------------------------------ PARSE ------------------------------------*/
void parse_args(int argc,char** argv)
{
	int option_index, c;
	char* endian_convert = NULL;

	static struct option long_options[] = 
	{
		{"vv", no_argument, 0, 'v'},
		{"v", no_argument, 0, 'w'},
		{"help", no_argument, 0, 'h'},
		{"h", no_argument, 0, 'i'},
		{"u", required_argument, 0, 'u'},
		{"UTF=", required_argument, 0, 't'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long_only(argc, argv, "", long_options, &option_index)) != -1)
	{
		switch(c)
		{ 
			case 'h':
				print_help();
				break;
			case 'i':
				print_help();
				break;
			case 'v':
				verbose = 2;
				break;
			case 'w':
				if(verbose == 1) verbose = 2;
				
				else verbose = 1;
				
				break;
			case 'u':
				endian_convert = optarg;
				o_enc = endian_convert;
				break;
			case 't':
				endian_convert = optarg;
				o_enc = endian_convert;
				break;
			case '?':

			default:
				fprintf(stderr, "Unrecognized argument.\n");
				print_help();
				break;
		}
	}

	if(optind < argc)
	{
		filename = argv[optind];
		optind++;
		if(optind < argc)
		{
			out_file = argv[optind];
			if(strcmp(filename, out_file) == 0)
			{
				fprintf(stderr,"Input and output files are the same.\n");
				print_help();
			}
		}
	}
	
	if(endian_convert == NULL)
	{
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
	}

	if(strcmp(endian_convert, "16LE") == 0) conversion = LITTLE;
	
	else if(strcmp(endian_convert, "16BE") == 0) conversion = BIG;
	
	else if(strcmp(endian_convert, "8") == 0) conversion = EIGHT;
	
	else quit_converter(NO_FD);
}

/*------------------------------ PRINT ------------------------------------*/
void print_help(void) 
{
	int i;
	for(i = 0; i < 13; i++)
	{
		fprintf(stderr,RED"%s"RESET, USAGE[i]); 
	}
	quit_converter(NO_FD);
}

/*------------------------------ PRINT VERBOSE----------------------------*/
void print_verbose(int verbose)
{
	struct stat st;
	int size = 0;
	char hostname[1024];
	char *file= filename;
	char full_path [4098];
	char *path_result;
	stat(filename, &st);
	size = st.st_size;
	hostname[1023] = '\0';
	gethostname(hostname, 1023);

	path_result = realpath(file, full_path);

	if(verbose == 1)
	{
		fprintf(stderr,BLU "Input file path:" GRN "%s\n" RESET, path_result);
		fprintf(stderr,BLU"Input file size: "GRN "%d kb\n" RESET, (int)round(size / 1000.0));
		fprintf(stderr,BLU"Input file encoding: "GRN"%s\n" RESET, s_enc);
		fprintf(stderr,BLU"Output encoding: UTF-"GRN"%s\n"RESET, o_enc);
		fprintf(stderr,BLU"Hostmachine: "GRN"%s\n"RESET, hostname);
		fprintf(stderr,BLU"Operating System: "GRN"%s\n"RESET, os);
	}
	else if(verbose == 2)
	{
		fprintf(stderr,BLU"Input file size: "GRN"%d kb\n"RESET, (int)round(size / 1000.0));
		fprintf(stderr,BLU"Input file path: "GRN"%s\n"RESET, path_result);
		fprintf(stderr,BLU"Input file encoding: "GRN"%s\n"RESET, s_enc);
		fprintf(stderr,BLU"Output encoding: UTF-"GRN"%s\n"RESET, o_enc);
		fprintf(stderr,BLU"Hostmachine: "GRN"%s\n"RESET, hostname);
		fprintf(stderr,BLU"Operating System: "GRN"%s\n"RESET, os);
		fprintf(stderr,BLU"Reading: real = "GRN"%f, user = %f, sys = %f\n"RESET, total_read_real, total_read_user, total_read_sys);
		fprintf(stderr,BLU"Converting: real = "GRN"%f, user = %f, sys = %f\n"RESET, total_conv_real, total_conv_user, total_conv_sys);
		fprintf(stderr,BLU"Writing: real = "GRN"%f, user = %f, sys = %f\n"RESET, total_write_real, total_write_user, total_write_sys);
		fprintf(stderr,BLU"ASCII: "GRN"%d%%\n"RESET,(int)round((total_ascii / total_glyphs) * 100));
		fprintf(stderr,BLU"Surrogates: "GRN"%d%%\n"RESET, (int)round((total_surrogate / total_glyphs)*100));
		fprintf(stderr,BLU"Glyphs: "GRN"%d\n"RESET, (int)total_glyphs);
	}
}

/*------------------------------ QUIT ------------------------------------*/
void quit_converter(int fd)
{
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD)
		close(fd);
	exit(EXIT_FAILURE);	
}

