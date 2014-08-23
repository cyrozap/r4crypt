/*
Copyright (C) 2007  Michael "Chishm" Chisholm
http://chishm.drunkencoders.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifdef __APPLE__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <string.h>
#include <stdio.h>

#include "address_seeds.h"
#include "stream_seeds.h"

unsigned char addrCypher (unsigned char val, int addr) 
{
	int byteOffset = addr % BYTE_ADDR_RANGE;
	int sectorOffset = addr / BYTE_ADDR_RANGE;

	val ^= ADDRESS_SEEDS [byteOffset][0];

	for (int i = 1; i < SECTOR_ADDR_RANGE; i++) {
		if ((sectorOffset & (1 << (i-1))) != 0) {
			val ^= ADDRESS_SEEDS [byteOffset][i];
		}
	}

	return val;
}

void usage (const char* progName)
{
	printf ("Usage: %s [-d|-e] <in_file> <out_file>\n");
}

int main(int argc, char* argv[])
{
	printf ("r4crypt version 1.0 - Copyright (C) 2007 Michael \"Chishm\" Chisholm\n");
	if (argc != 4) {
		usage (argv[0]);
		return -1;
	}

	enum {decrypt, encrypt} mode;

	if (strcmp ("-d", argv[1]) == 0) {
		mode = decrypt;
	} else {
		mode = encrypt;
	}

	FILE* inFile = fopen (argv[2], "rb");
	FILE* outFile = fopen (argv[3], "wb");

	fseek (inFile, 0, SEEK_END);
	int inSize = ftell (inFile);
	fseek (inFile, 0, SEEK_SET);

	int bufferSize = inSize + BYTE_STREAM_RANGE;

	unsigned char *inData = (unsigned char*) malloc(bufferSize);
	fread (inData, 1, inSize, inFile);
	fclose (inFile);

	unsigned char *outData = (unsigned char*) malloc(bufferSize);
	memset (outData, 0, inSize);

	const int percentGap = 5;
	int prevPercent = 0;
	int percent;

	if (mode == decrypt) {
		printf ("Decrypting %s into %s\n", argv[2], argv[3]);
		for (int pos = 0; pos < inSize; pos++) {
			for (int bit = 0; bit < BIT_STREAM_RANGE; bit++) {
				if (inData[pos] & (1 << bit)) {
					for (int curPosOffset = 1; curPosOffset < (BYTE_STREAM_RANGE - (pos % BYTE_STREAM_RANGE)); curPosOffset++) {
						outData[pos + curPosOffset] ^= STREAM_SEEDS [bit][curPosOffset];
					}
				}
			}
			outData[pos] ^= addrCypher (inData[pos], pos);

			percent = pos * (100 / percentGap) / inSize;
			if (percent != prevPercent) {
				printf ("%02d%% ", percent * percentGap);
				prevPercent = percent;
			}
		}
	} else {
		printf ("Encrypting %s into %s\n", argv[2], argv[3]);
		int prevPercent = 0;
		for (int pos = 0; pos < inSize; pos++) {
			outData[pos] ^= addrCypher (inData[pos], pos);
			for (int bit = 0; bit < BIT_STREAM_RANGE; bit++) {
				if (outData[pos] & (1 << bit)) {
					for (int curPosOffset = 1; curPosOffset < (BYTE_STREAM_RANGE - (pos % BYTE_STREAM_RANGE)); curPosOffset++) {
						outData[pos + curPosOffset] ^= STREAM_SEEDS [bit][curPosOffset];
					}
				}
			}

			percent = pos * (100 / percentGap) / inSize;
			if (percent != prevPercent) {
				printf ("%02d%% ", percent * percentGap);
				prevPercent = percent;
			}
		}
	}
	printf ("100%%\n");

	fwrite (outData, 1, inSize, outFile);

	fclose (outFile);

	free (inData);
	free (outData);

	return 0;
}

/*
This code is for generating the address seeds. 
Requires a 4MiB data file consisting of all 00s to be 
fed through the decryptor of the R4. The result is used
to calculate the seeds for the address cypher.

char getFileByte (FILE* file, int offset) 
{
	int byte;
	fseek (file, offset, SEEK_SET);
	byte = fgetc (file);
	if (byte < 0) {
		return 0;
	} 
	return (char)byte;
}

char addressSeeds [BYTE_ADDR_RANGE][SECTOR_ADDR_RANGE];
int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf ("Usage: %s <in file> <out table>\n", argv[0]);
		return -1;
	}

	FILE* in = fopen (argv[1], "rb");
	FILE* out = fopen (argv[2], "wt");

	fseek (in, 0, SEEK_END);
	if (ftell (in) < (4*1024*1024)) {
		printf ("In file must be at least 4MiB\n");
		return -1;
	}

	for (int byteAddr = 0; byteAddr < BYTE_ADDR_RANGE; byteAddr++) {
		char baseByte = getFileByte (in, byteAddr);
		addressSeeds[byteAddr][0] = baseByte;
		for (int sectorAddr = 1; sectorAddr < SECTOR_ADDR_RANGE; sectorAddr++) {
			int sectorOffset = (1 << (sectorAddr-1)) * BYTE_ADDR_RANGE;
			addressSeeds[byteAddr][sectorAddr] = baseByte ^ getFileByte (in, byteAddr + sectorOffset);
		}
	}

	fprintf (out, "const char ADDRESS_SEEDS [BYTE_ADDR_RANGE][SECTOR_ADDR_RANGE] = {\n");
	for (int byteAddr = 0; byteAddr < BYTE_ADDR_RANGE; byteAddr++) {
		fprintf (out, "\t{");
		for (int sectorAddr = 0; sectorAddr < SECTOR_ADDR_RANGE; sectorAddr++) {
			if (sectorAddr > 0) {
				fprintf (out, ", ");
			}
			fprintf (out, "0x%02X", (addressSeeds[byteAddr][sectorAddr] & 0xFF));
		}
		if (byteAddr < BYTE_ADDR_RANGE - 1) {
			fprintf (out, "},\n");
		} else {
			fprintf (out, "}\n");
		}
	}
	fprintf (out, "};\n");

	fclose (in);
	fclose (out);


	return 0;
}*/

