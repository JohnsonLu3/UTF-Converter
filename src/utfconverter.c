#include "utfconverter.h"

char* filename = NULL;
char* outputname = NULL;
int fd_out;
int verbosity = 0;
int en = -1;
int n = 1;
float glyph_COUNT = 0;
float surroPair_COUNT = 0;
float ascii_COUNT = 0;

float read_R = 0;
float read_U = 0;
float read_S = 0;

float conv_R = 0;
float conv_U = 0;
float conv_S = 0;

float writ_R = 0;
float writ_U = 0;
float writ_S = 0;

clock_t st_time;
clock_t en_time;
struct tms st_cpu;
struct tms en_cpu;

endianness conversion;
endianness source;


int main(int argc ,char** argv){
	/* After calling parse_args(), filename and conversion should be set. */
	int fd;
	int rv;
	Glyph bomG = {0};

	unsigned int buf[2];
	Glyph* glyph = malloc(sizeof(Glyph));

	if(*(char *)&n ==1){
		/*os is little endian*/
		en = 0;
	}else{
		/*os is big endian*/
		en = 1;

	}
	
	if(glyph == NULL){
		free(glyph);
		quit_converter(NO_FD);
		exit(EXIT_FAILURE);
	} 

	parse_args(argc, argv);
	fd = -1;
	fd_out = -1;

	if(filename != NULL){
		fd = open(filename, O_RDONLY); 
	}else{
		quit_converter(NO_FD);
		exit(EXIT_FAILURE);
	}

	/*Opens output file if given*/
	if (outputname != NULL){
		fd_out = open(outputname, O_RDONLY);
		if(fd_out == -1){
			close(fd_out);

			fd_out = open(outputname, O_RDWR |O_CREAT | O_TRUNC,
    		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    		close(fd_out);
		}

		fd_out = open(outputname, O_RDWR);
		checkOutBom();
		close(fd_out);
		fd_out = open(outputname, O_APPEND | O_WRONLY);


	}else{
		/*No output file given or invalid*/
		/*write BOM*/
		if(conversion == BIG){
			/*0xfe 0xff*/
			bomG.bytes[0] = 0xfe;
			bomG.bytes[1] = 0xff;

			write_glyph(&bomG);
			
		}else if(conversion == LITTLE){
			/*0xff 0xfe*/
			bomG.bytes[0] = 0xff;
			bomG.bytes[1] = 0xfe;

			write_glyph(&bomG);			
		}
		close(fd_out);
	}

	/*EXIT IF FD IS INVALID*/
	if(fd == -1){
		quit_converter(NO_FD);
		exit(EXIT_FAILURE);
	}

	/*Invalid output file*/
	if(fd_out == -1){

	}

	buf[0] = 0;
	buf[1] = 0; 
	
	rv = '0'; 

	
	/* Handle BOM bytes for UTF16 specially.*/
    /* Read our values into the first and second elements. */

	/*Start Time*/
	st_time = (float)times(&st_cpu);
	
	if((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1){ 
		unsigned int swap;
		en_time = (float)times(&en_cpu);

		read_R += (en_time - st_time);
		read_U += (en_cpu.tms_utime - st_cpu.tms_utime);
		read_S += (en_cpu.tms_stime - st_cpu.tms_stime);
		

		swap = 0;

		if(en == 1){
			swap = ((buf[0]>>24)&0xff) | 
	               ((buf[0]<<8)&0xff0000) | 
	               ((buf[0]>>8)&0xff00) | 
	               ((buf[0]<<24)&0xff000000); 
			buf[0] = swap;
			swap = ((buf[1]>>24)&0xff) | 
	               ((buf[1]<<8)&0xff0000) | 
	               ((buf[1]>>8)&0xff00) | 
	               ((buf[1]<<24)&0xff000000); 
			buf[1] = swap;
		}

		if(buf[0] == 0xff && buf[1] == 0xfe){
			/*file is little endian*/
			source = LITTLE;
		} else if(buf[0] == 0xfe && buf[1] == 0xff){
			/*file is big endian*/
			source = BIG; 	
		} else if(buf[0] == 0xef && buf[1] == 0xbb){
			
			read(fd, &buf[0], 1);
			if(en == 1){
				buf[0] = buf[0] >> 24;
			}

			if(buf[0] == 0xbf){
				/**File is UTF8*/
				source = UTF8;
			}

		} else {
			/*file has no BOM*/
			free(&glyph->bytes); 
			fprintf(stderr, "File has no BOM.\n");
			quit_converter(NO_FD);
			exit(EXIT_FAILURE); 
		}
		
		memset(glyph, 0, sizeof(Glyph));
	}


	/* Now deal with the rest of the bytes.*/
	while(source != UTF8){

		st_time = times(&st_cpu);
		if((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1){
			Glyph glyph2Write = {0};

			/*Get Read times*/
			en_time = times(&en_cpu);

			read_R += (en_time - st_time);
			read_U += (en_cpu.tms_utime - st_cpu.tms_utime);
			read_S += (en_cpu.tms_stime - st_cpu.tms_stime);


			/*Get convert times*/
			st_time = times(&en_cpu);

			glyph2Write = *fill_glyph(glyph, buf, source, &fd);

			en_time = times(&en_cpu);

			conv_R += (en_time - st_time);
			conv_U += (en_cpu.tms_utime - st_cpu.tms_utime);
			conv_S += (en_cpu.tms_stime - st_cpu.tms_stime);

			if(conversion == glyph2Write.end){
				write_glyph(&glyph2Write);	
			}else{
				write_glyph(swap_endianness(&glyph2Write));
			}
				 
			memset(glyph, 0, sizeof(Glyph));

		}else{
			break;
		}

	}

	if(source == UTF8){
		while(1){
			if((rv = read(fd, &buf[0], 1)) == 1){
				Glyph glyph2Wrt = {0};

				if(en == 1){
					buf[0] = buf[0] >> 24;
				}

				glyph->bytes[0] = buf[0];

				glyph2Wrt = *convert(glyph, conversion, fd);

				if(glyph2Wrt.end == LITTLE){
					glyph2Wrt = *swap_endianness(&glyph2Wrt);
				}

				write_glyph(&glyph2Wrt);

				memset(glyph, 0, sizeof(Glyph));

			}else{
				break;
			}
		}
	}


	if(verbosity > 0){
		print_verbosity(verbosity);
	}

	quit_converter(NO_FD);
	return EXIT_SUCCESS;
}

int checkOutBom(){
	unsigned int buffer[3] = {0};
	Glyph bomG = {0};
	int rv = 0;

	if((rv = read(fd_out, &buffer[0], 1)) == 1 &&
	 (rv = read(fd_out, &buffer[1], 1)) == 1){ 
		/* File Not Empty*/
		if(en == 1){
			buffer[0] = buffer[0] >> 24;
			buffer[1] = buffer[1] >> 24;
		}
			if((buffer[0] == 0xff && buffer[1] == 0xfe) ||
			   (buffer[0] == 0xfe && buffer[1] == 0xff)){
				/*BOM exist*/

				glyph_COUNT += 1;

				if(buffer[0] == 0xff && conversion != LITTLE){
					/*INVALID BOM*/
					/*fprintf(stderr, "INVALID BOM\n" );*/
					quit_converter(NO_FD);
					exit(EXIT_FAILURE);
				}else if(buffer[0] == 0xfe && conversion != BIG){
					/*INVALID BOM*/
					/*fprintf(stderr, "INVALID BOM\n" );*/
					quit_converter(NO_FD);
					exit(EXIT_FAILURE);
				}

				return 0;
			}else{
				/*write BOM*/
				if(conversion == BIG){
					/*0xfe 0xff*/
					bomG.bytes[0] = 0xfe;
					bomG.bytes[1] = 0xff;

					write_glyph(&bomG);
					
				}else if(conversion == LITTLE){
					/*0xff 0xfe*/
					bomG.bytes[0] = 0xff;
					bomG.bytes[1] = 0xfe;

					write_glyph(&bomG);			
				}
				return 1;
			}

		}else{
			/* File is EMPTY*/
			/*write BOM*/
				if(conversion == BIG){
					/*0xfe 0xff*/
					bomG.bytes[0] = 0xfe;
					bomG.bytes[1] = 0xff;

					write_glyph(&bomG);
					
				}else if(conversion == LITTLE){
					/*0xff 0xfe*/
					bomG.bytes[0] = 0xff;
					bomG.bytes[1] = 0xfe;

					write_glyph(&bomG);			
				}
			return 1;
		}
	return 0;
}


Glyph* swap_endianness(Glyph* glyph){
	/* Use XOR to be more efficient with how we swap values. */
	int temp;
	temp = glyph->bytes[0];
	glyph->bytes[0] = glyph->bytes[1];
	glyph->bytes[1] = temp;
	if(glyph->surrogate){  
	/* If a surrogate pair, swap the next two bytes. */
		temp = glyph->bytes[2];
		glyph->bytes[2] = glyph->bytes[3];
		glyph->bytes[3] = temp;
	}
	glyph->end = conversion;
	return glyph;
}

Glyph* fill_glyph(Glyph* glyph, unsigned int data[2], endianness end, int* fd){
	unsigned int bits;

	if(en == 1){
		data[0] = data[0] >> 24;
		data[1] = data[1] >> 24;
	}

	glyph->bytes[0] = data[0];
	glyph->bytes[1] = data[1];

	bits = 0; 
	bits |= (data[FIRST] + (data[SECOND] << 8));
	/*
	if((data[0] > 0x0) && (data[0] <= 0x7f) && (data[1] == 0x0) && (source == LITTLE)){
		ascii_COUNT += 1;
	}else if((data[1] > 0x0) && (data[1] <= 0x7f) && (data[0] == 0x0) && (source == BIG)){
		ascii_COUNT += 1;
	}*/

	/* Check high surrogate pair using its special value range.*/
	if(bits > 0x000F && bits < 0xF8FF){ 
		
		if(read(*fd, &data[FIRST], 1) == 1 && read(*fd, &data[SECOND], 1) == 1){	
			
			en_time = times(&en_cpu);

			read_R += (en_time - st_time);
			read_U += (en_cpu.tms_utime - st_cpu.tms_utime);
			read_S += (en_cpu.tms_stime - st_cpu.tms_stime);

			bits = 0; 

		if(en == 1){
			data[0] = data[0] >> 24;
			data[1] = data[1] >> 24;
		}


		bits |= (data[FIRST] + (data[SECOND] << 8));
		
		if(bits > 0xDAAF && bits > 0x00FF){ 
			/* Check low surrogate pair.*/
				surroPair_COUNT += 1;
				glyph->surrogate = true; 
			} else {
				ascii_COUNT += 1;
				lseek(*fd, -OFFSET, SEEK_CUR); 
				glyph->surrogate = false;
			}
		}
	}



	if(!glyph->surrogate){
		glyph->bytes[THIRD] = glyph->bytes[FOURTH] |= 0;
	} else {
		glyph->bytes[THIRD] = data[FIRST]; 
		glyph->bytes[FOURTH] = data[SECOND];
	}
	glyph->end = end;

	return glyph;
}

void write_glyph(Glyph* glyph){

	glyph_COUNT += 1;

	if(fd_out != -1){
		if(!glyph->surrogate){
			
			st_time = times(&en_cpu);

			write(fd_out, glyph->bytes, NON_SURROGATE_SIZE);

			en_time = times(&en_cpu);

			writ_R += (en_time - st_time);
			writ_U += (en_cpu.tms_utime - st_cpu.tms_utime);
			writ_S += (en_cpu.tms_stime - st_cpu.tms_stime);

		} else {

			st_time = times(&en_cpu);

			write(fd_out, glyph->bytes, SURROGATE_SIZE);

			en_time = times(&en_cpu);

			writ_R += (en_time - st_time);
			writ_U += (en_cpu.tms_utime - st_cpu.tms_utime);
			writ_S += (en_cpu.tms_stime - st_cpu.tms_stime);
		}

	}else{

		if(!glyph->surrogate){

			st_time = times(&en_cpu);

			write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);

			en_time = times(&en_cpu);

			writ_R += (en_time - st_time);
			writ_U += (en_cpu.tms_utime - st_cpu.tms_utime);
			writ_S += (en_cpu.tms_stime - st_cpu.tms_stime);
		} else {

			st_time = times(&en_cpu);

			write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);

			en_time = times(&en_cpu);

			writ_R += (en_time - st_time);
			writ_U += (en_cpu.tms_utime - st_cpu.tms_utime);
			writ_S += (en_cpu.tms_stime - st_cpu.tms_stime);
		}		
	}

}

void print_verbosity(int level){

	struct stat st = {0};
	struct utsname OSdata;
	int size = 0;
	int asciiPer = 0;
	int surroPair = 0;
	int glyphCnt = 0;
	char input_encoding[10] = "";
	char output_encoding[10] = "";
	char hostname[128];
	char absPath [PATH_MAX+1];
	char *pathPtr;


	gethostname(hostname, sizeof hostname);
	uname(&OSdata); 

	pathPtr = realpath(filename,absPath);


	if(source == LITTLE){
		strcpy(input_encoding,"UTF-16LE");
	}else if(source == BIG){
		strcpy(input_encoding, "UTF-16BE");
	}else if(source == UTF8){
		strcpy(input_encoding, "UTF-8");
	}

	if(conversion == LITTLE){
		strcpy(output_encoding, "UTF-16LE");
	}else if(conversion == BIG){
		strcpy(output_encoding, "UTF-16BE");
	}

	stat(filename, &st);
	size = roundf(st.st_size / 1000); 
	if(level > 0 && level < 3){
		/*Print level 1*/
		fprintf(stderr, "Input file size: %d kb\n", size);
		fprintf(stderr, "Input file path: %s\n", pathPtr);
		fprintf(stderr, "Input file encoding: %s\n", input_encoding);
		fprintf(stderr, "Output encoding: %s\n", output_encoding);
		fprintf(stderr, "Hostmachine: %s\n", hostname);
		fprintf(stderr, "Operating System: %s\n", OSdata.sysname);

	}

	if(level == 2){
		/*Print level 2*/

		glyphCnt = glyph_COUNT;

		surroPair = roundf((surroPair_COUNT / glyph_COUNT) * 100);
		asciiPer = roundf((ascii_COUNT / glyph_COUNT) * 100);

		fprintf(stderr, "Reading: real=%f, user=%f, sys=%f\n", read_R, read_U, read_S);
		fprintf(stderr, "Converting: real=%f, user=%f, sys=%f\n", conv_R, conv_U, conv_S);
		fprintf(stderr, "Writing: real=%f, user=%f, sys=%f\n", writ_R, writ_U, writ_S);
		fprintf(stderr, "ASCII: %d%%\n", asciiPer);
		fprintf(stderr, "Surrogates: %d%%\n", surroPair);
		fprintf(stderr, "Glyphs: %d\n", glyphCnt);
	}
}

void parse_args(int argc, char** argv){
	int option_index, c;
	char* endian_convert = NULL;

	#include "struct.txt"

	while(1){

		c = getopt_long(argc, argv, "vhu:", long_options, &option_index);

		if(c == -1){
			break;
		}
		/* If getopt() returns with a valid (its working correctly) 
		 * return code, then process the args! */
		switch(c){ 
			case 'h':
				print_help();
				break;
			case 'v':
				if(verbosity >= 1){
					verbosity = 2;
				}else{
					verbosity = 1;
				}
				break;
			case 'u':
				endian_convert = optarg;
				break;

			default:
				fprintf(stderr, "Unrecognized argument.\n");
				quit_converter(NO_FD);
				exit(EXIT_FAILURE);
				break;
		}
	}

	if(optind < argc){
		filename = (char*) malloc(strlen(argv[optind]) + sizeof(char));
		strcpy(filename, argv[optind]);
		optind++;
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
	}

	if(optind < argc){
		/*Output File was given*/
		outputname = (char*) malloc(strlen(argv[optind]) + sizeof(char));
		strcpy(outputname, argv[optind]);
	}

	if(filename != NULL && outputname != NULL){
		if(strcmp(filename, outputname) == 0){
			quit_converter(NO_FD);
			exit(EXIT_FAILURE);
		}
	}

	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
	}

	if(strcmp(endian_convert, "16LE") == 0){ 
		conversion = LITTLE;
	} else if(strcmp(endian_convert, "16BE") == 0){
		conversion = BIG;
	} else if(strcmp(endian_convert, "UTF8") == 0){
		conversion = UTF8;
	} else {
		/*Not 16LE or 16Be or UTF8*/
		quit_converter(NO_FD);
		exit(EXIT_FAILURE);
	}
}

int swapEnd(int byte2swap){
	int byte = byte2swap;
	unsigned int swap;

	if(en == 1){
		swap = ((byte2swap>>24)&0xff) | 
               ((byte2swap<<8)&0xff0000) | 
               ((byte2swap>>8)&0xff00) | 
               ((byte2swap<<24)&0xff000000); 
		byte = swap;
	}

	return byte;
}


Glyph* convert(Glyph* glyph, endianness end, int fd){

	int rv;
	int MSB;
	int byte_Cnt;
	unsigned int buf[4] = {0};
	unsigned int code = 0;


	rv = 0;
	MSB = -1;
	byte_Cnt = 1;

		
	buf[0] = glyph->bytes[0];

		/*[xx]yyyyy*/

		MSB = buf[0] >> 7;

		/*If MSB is 1 then its a multi byte glyph*/
		if(MSB == 0x1){
			/*[xxx]yyyyy*/
			MSB = buf[0] >> 5;

			/*0x6 = 110*/
			if(MSB == 0x6){
				byte_Cnt = 2;
			}else{
				/*[xxxx]yyyy*/
				MSB = buf[0] >> 4;

				/*0xE = 1110*/
				if(MSB == 0xE){
					byte_Cnt = 3;
				}else{
					/*[xxxxx]yyy*/
					MSB = buf[0] >> 3;

					/*0x1E = 11110*/
					if(MSB == 0x1E){
						byte_Cnt = 4;
						/*MAX BYTES*/
					}
				}

			}
		/*Byte count DONE*/
		if(byte_Cnt == 2){
			/* read another byte TOTAL 2*/
			if((rv = read(fd, &buf[1], 1)) == 1){
				int bits1;
				int bits2;

				if(en == 1){
					buf[1] = buf[1] >> 24;
				}	

				bits1 = 0;
				bits2 = 0;

				bits1 = buf[0] ^ 0xC0;
				bits2 = buf[1] ^ 0x80;

				code = (bits1 << 6) | bits2;

				bits1 = code >> 8; /*0xCCbb -> 0xCC*/
				bits2 = (bits1 << 8) ^ code;

				glyph->bytes[0] = bits1;
				glyph->bytes[1] = bits2;
				glyph->surrogate = false; 

			}

		}else if(byte_Cnt == 3){
			/*read 2 more bytes TOTAL 3*/
			if((rv = read(fd, &buf[1], 1)) == 1 &&
				(rv = read(fd, &buf[2], 1)) == 1){
				int bits1 = 0;
				int bits2 = 0;
				int bits3 = 0;

				if(en == 1){
					buf[1] = buf[1] >> 24;
					buf[2] = buf[2] >> 24;
				}	
	

				bits1 = buf[0] ^ 0xE0;
				bits2 = buf[1] ^ 0x80;
				bits3 = buf[2] ^ 0x80;

				code = (bits1 << 12) | (bits2 << 6) | bits3;

				bits1 = code >> 8;
				bits2 = (bits1 << 8) ^ code;
				bits3 = code >> 16;

				glyph->bytes[0] = bits1;
				glyph->bytes[1] = bits2;
				glyph->bytes[2] = bits3;
				glyph->bytes[3] = 0x0;
				glyph->surrogate = false; 

			}

		}else if(byte_Cnt == 4){
			/*Read 3 more bytes TOTAL 4*/
			if((rv = read(fd, &buf[1], 1)) == 1&&
				(rv = read(fd, &buf[2], 1)) == 1&&
				(rv = read(fd, &buf[3], 1)) == 1){

				int bits1 = 0;
				int bits2 = 0;
				int bits3 = 0;
				int bits4 = 0;
				unsigned int codePrime = 0;
				unsigned int codeH = 0;
				unsigned int codeL = 0;

				if(en == 1){
					buf[1] = buf[1] >> 24;
					buf[2] = buf[2] >> 24;
					buf[3] = buf[3] >> 24;
				}	


				bits1 = buf[0] ^ 0xF0;
				bits2 = buf[1] ^ 0x80;
				bits3 = buf[2] ^ 0x80;
				bits4 = buf[3] ^ 0x80;

				code = (bits1 << 18) | (bits2 << 12) | (bits3 << 6) | bits4;
				
				codePrime = code - 0x10000;
				codeH = codePrime >> 10;
				codeL = codePrime & 0x3ff;

				/*surrogates*/
				codeH = 0xD800 + codeH;
				codeL = 0xDC00 + codeL;



				bits1 = codeH >> 8; /*0xCCbb -> 0xCC*/
				bits2 = (bits1 << 8) ^ codeH;

				glyph->bytes[0] = bits1;
				glyph->bytes[1] = bits2;

				bits3 = codeL >> 8; /*0xCCbb -> 0xCC*/
				bits4 = (bits3 << 8) ^ codeL;

				glyph->bytes[2] = bits3;
				glyph->bytes[3] = bits4;

				surroPair_COUNT += 1;
				glyph->surrogate = true; 

			}

		}else{
			/*byte cnt > 4 not supported*/
			quit_converter(fd);
			exit(EXIT_FAILURE);
		}

		}else{
			/* 1 Byte Count*/
			ascii_COUNT += 1;
			glyph->bytes[0] = 0x0;
			glyph->bytes[1] = buf[0];
			glyph->bytes[2] = 0x0;
			glyph->bytes[3] = 0x0;
			glyph->surrogate = false;
		}
	

	glyph->end = end;

	return glyph;
}

void print_help(void) {
	int i;
	i = 0;
	for(; i < 16; i++){
		printf("%s", USAGE[i]); 
	}
	quit_converter(NO_FD);
	exit(EXIT_SUCCESS);
}

void quit_converter(int fd)
{
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD){
		close(fd);
	}
}
