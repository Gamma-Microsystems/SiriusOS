/**
 * @file  kernel/misc/gzip.c
 * @brief Gzip/DEFLATE decompression.
 *
 * Provides decompression for ramdisks.
 * Based on the same approach to DEFLATE decompression as libraries
 * like "tinf", this is a slow-but-simple implementation of Huffman
 * decoding. The kernel version operates directly on two pointers,
 * @c gzip_inputPtr and @c gzip_outputPtr. For a more robust API,
 * see the userspace version.
 *
 * @copyright
 * This file is part of SiriusOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2020-2021 K. Lange
 */
#include <stdint.h>
#include <stddef.h>

static uint8_t bit_buffer = 0;
static char buffer_size = 0;

uint8_t * gzip_inputPtr = NULL;
uint8_t * gzip_outputPtr = NULL;

__attribute__((always_inline))
static inline uint8_t read_byte(void) {
	return *gzip_inputPtr++;
}

__attribute__((always_inline))
static inline void _write(unsigned int sym) {
	*gzip_outputPtr++ = sym;
}

/**
 * Decoded Huffman table
 */
struct huff {
	uint16_t counts[16];   /* Number of symbols of each length */
	uint16_t symbols[288]; /* Ordered symbols */
};

/**
 * Fixed Huffman code tables, generated later.
 */
struct huff fixed_lengths;
struct huff fixed_dists;

/**
 * Read a little-endian short from the input.
 */
static uint16_t read_16le(void) {
	uint16_t a, b;
	a = read_byte();
	b = read_byte();
	return (a << 0) | (b << 8);
}

/**
 * Read a single bit from the source.
 * Fills the byte buffer with one byte when it runs out.
 */
static _Bool read_bit(void) {

	/* When we run out of bits... */
	if (buffer_size == 0) {
		/* Refill from the next input byte */
		bit_buffer = read_byte();
		/* And restore bit buffer size to 8 bits */
		buffer_size = 8;
	}

	/* Get the next available bit */
	int out = bit_buffer & 1;

	/* Shift the bit buffer forward */
	bit_buffer >>= 1;

	/* There is now one less bit available */
	buffer_size--;

	return out;
}

/**
 * Read multible bits, in bit order, from the source.
 */
static uint32_t read_bits(unsigned int count) {
	uint32_t out = 0;
	for (unsigned int bit = 0; bit < count; bit++) {
		/* Read one bit at a time, from least to most significant */
		out |= (read_bit() << bit);
	}
	return out;
}

/**
 * Build a Huffman table from an array of lengths.
 */
static void build_huffman(const uint8_t * lengths, size_t size, struct huff * out) {

	uint16_t offsets[16];
	unsigned int count = 0;

	/* Zero symbol counts */
	for (unsigned int i = 0; i < 16; ++i) out->counts[i] = 0;

	/* Count symbols */
	for (unsigned int i = 0; i < size; ++i) out->counts[lengths[i]]++;

	/* Special case... */
	out->counts[0] = 0;

	/* Figure out offsets */
	for (unsigned int i = 0; i < 16; ++i) {
		offsets[i] = count;
		count += out->counts[i];
	}

	/* Build symbol ordering */
	for (unsigned int i = 0; i < size; ++i) {
		if (lengths[i]) out->symbols[offsets[lengths[i]]++] = i;
	}
}

/**
 * Build the fixed Huffman tables
 */
static void build_fixed(void) {
	/* From 3.2.6:
	 * Lit Value    Bits        Codes
	 * ---------    ----        -----
	 *   0 - 143     8          00110000 through
	 *                          10111111
	 * 144 - 255     9          110010000 through
	 *                          111111111
	 * 256 - 279     7          0000000 through
	 *                          0010111
	 * 280 - 287     8          11000000 through
	 *                          11000111
	 */
	uint8_t lengths[288];
	for (int i = 0; i < 144; ++i)   lengths[i] = 8;
	for (int i = 144; i < 256; ++i) lengths[i] = 9;
	for (int i = 256; i < 280; ++i) lengths[i] = 7;
	for (int i = 280; i < 288; ++i) lengths[i] = 8;
	build_huffman(lengths, 288, &fixed_lengths);

	/* Continued from 3.2.6:
	 * Distance codes 0-31 are represented by (fixed-length) 5-bit
	 * codes, with possible additional bits as shown in the table
	 * shown in Paragraph 3.2.5, above.  Note that distance codes 30-
	 * 31 will never actually occur in the compressed data.
	 */
	for (int i = 0; i < 30; ++i) lengths[i] = 5;
	build_huffman(lengths, 30, &fixed_dists);
}


/**
 * Decode a symbol from the source using a Huffman table.
 */
static int decode(const struct huff * huff) {
	int count = 0, cur = 0;
	for (int i = 1; cur >= 0; i++) {
		cur = (cur << 1) | read_bit(); /* Shift */
		count += huff->counts[i];
		cur -= huff->counts[i];
	}
	return huff->symbols[count + cur];
}

struct huff_ring {
	size_t pointer;
	uint8_t data[32768];
};

static struct huff_ring data = {0, {0}};

/**
 * Emit one byte to the output, maintaining the ringbuffer.
 * The ringbuffer ensures we can always look back 32K bytes
 * while keeping output streaming.
 */
static void emit(unsigned char byte) {
	data.data[data.pointer++] = byte;
	data.pointer &= 0x7FFF;
	_write(byte);
}

/**
 * Look backwards in the output ring buffer.
 */
static void peek(unsigned int offset) {
	data.data[data.pointer] = data.data[(data.pointer + 0x8000 - offset) & 0x7FFF];
	_write(data.data[data.pointer++]);
	data.pointer &= 0x7FFF;
}

/**
 * Decompress a block of Huffman-encoded data.
 */
static int inflate(const struct huff * huff_len, const struct huff * huff_dist) {

	/* These are the extra bits for lengths from the tables in section 3.2.5
	 *           Extra               Extra               Extra
	 *      Code Bits Length(s) Code Bits Lengths   Code Bits Length(s)
	 *      ---- ---- ------     ---- ---- -------   ---- ---- -------
	 *       257   0     3       267   1   15,16     277   4   67-82
	 *       258   0     4       268   1   17,18     278   4   83-98
	 *       259   0     5       269   2   19-22     279   4   99-114
	 *       260   0     6       270   2   23-26     280   4  115-130
	 *       261   0     7       271   2   27-30     281   5  131-162
	 *       262   0     8       272   2   31-34     282   5  163-194
	 *       263   0     9       273   3   35-42     283   5  195-226
	 *       264   0    10       274   3   43-50     284   5  227-257
	 *       265   1  11,12      275   3   51-58     285   0    258
	 *       266   1  13,14      276   3   59-66
	 */
	static const uint16_t lens[] = {
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51,
		59, 67, 83, 99, 115, 131, 163, 195, 227, 258
	};
	static const uint16_t lext[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,
		4, 5, 5, 5, 5, 0
	};

	/* Extra bits for distances....
	 *            Extra           Extra               Extra
	 *       Code Bits Dist  Code Bits   Dist     Code Bits Distance
	 *       ---- ---- ----  ---- ----  ------    ---- ---- --------
	 *         0   0    1     10   4     33-48    20    9   1025-1536
	 *         1   0    2     11   4     49-64    21    9   1537-2048
	 *         2   0    3     12   5     65-96    22   10   2049-3072
	 *         3   0    4     13   5     97-128   23   10   3073-4096
	 *         4   1   5,6    14   6    129-192   24   11   4097-6144
	 *         5   1   7,8    15   6    193-256   25   11   6145-8192
	 *         6   2   9-12   16   7    257-384   26   12  8193-12288
	 *         7   2  13-16   17   7    385-512   27   12 12289-16384
	 *         8   3  17-24   18   8    513-768   28   13 16385-24576
	 *         9   3  25-32   19   8   769-1024   29   13 24577-32768
	 */
	static const uint16_t dists[] = {
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
		513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
	};
	static const uint16_t dext[] = {
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
		10, 11, 11, 12, 12, 13, 13
	};

	while (1) {
		unsigned int symbol = decode(huff_len);
		if (symbol < 256) {
			emit(symbol);
		} else if (symbol == 256) {
			/* "The literal/length symbol 256 (end of data), ..." */
			break;
		} else {
			unsigned int length, distance, offset;
			symbol -= 257;
			length = read_bits(lext[symbol]) + lens[symbol];
			distance = decode(huff_dist);
			offset = read_bits(dext[distance]) + dists[distance];
			for (unsigned int i = 0; i < length; ++i) {
				peek(offset);
			}
		}
	}

	return 0;
}

/**
 * Decode a dynamic Huffman block.
 */
static void decode_huffman(void) {

	/* Ordering of code length codes:
	 * (HCLEN + 4) x 3 bits: code lengths for the code length
	 * alphabet given just above, in the order: ...
	 */
	static const uint8_t clens[] = {
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	unsigned int literals, distances, clengths;
	uint8_t lengths[320] = {0};

	literals  = 257 + read_bits(5); /* 5 Bits: HLIT ... 257 */
	distances = 1 + read_bits(5);   /* 5 Bits: HDIST ... 1 */
	clengths  = 4 + read_bits(4);   /* 4 Bits: HCLEN ... 4 */

	/* (HCLEN + 4) x 3 bits... */
	for (unsigned int i = 0; i < clengths; ++i) {
		lengths[clens[i]] = read_bits(3);
	}

	struct huff codes;
	build_huffman(lengths, 19, &codes);

	/* Decode symbols:
	 * HLIT + 257 code lengths for the literal/length alphabet...
	 * HDIST + 1 code lengths for the distance alphabet...
	 */
	unsigned int count = 0;
	while (count < literals + distances) {
		int symbol = decode(&codes);
		int rep = 0, length;
		switch (symbol) {
			case 16:
				/* 16: Copy the previous code length 3-6 times */
				rep = lengths[count-1];
				length = read_bits(2) + 3; /* The next 2 bits indicate repeat length */
				break;
			case 17:
				/* Repeat a code length of 0 for 3 - 10 times */
				length = read_bits(3) + 3; /* 3 bits of length */
				break;
			case 18:
				/* Repeat a code length of 0 for 11 - 138 times */
				length = read_bits(7) + 11; /* 7 bits of length */
				break;
			default:
				length = 1;
				rep = symbol;
				break;
		}
		while (length--) {
			lengths[count++] = rep;
		}
	}

	/* Build tables from lenghts decoded above */
	struct huff huff_len;
	build_huffman(lengths, literals, &huff_len);
	struct huff huff_dist;
	build_huffman(lengths + literals, distances, &huff_dist);

	inflate(&huff_len, &huff_dist);
}

/**
 * Decode an uncompressed block.
 */
static int uncompressed(void) {
	/* Reset byte alignment */
	bit_buffer = 0;
	buffer_size = 0;

	/* "The rest of the block consists of the following information:"
	 *    0   1   2   3   4...
	 *  +---+---+---+---+================================+
	 *  |  LEN  | NLEN  |... LEN bytes of literal data...|
	 *  +---+---+---+---+================================+
	 */
	uint16_t len = read_16le(); /* "the number of data bytes in the block" */
	uint16_t nlen = read_16le(); /* "the one's complement of LEN */

	/* Sanity check - does the ones-complement length actually match? */
	if ((nlen & 0xFFFF) != (~len & 0xFFFF)) {
		return 1;
	}

	/* Emit LEN bytes from the source to the output */
	for (int i = 0; i < len; ++i) {
		emit(read_byte());
	}

	return 0;
}

/**
 * Decompress DEFLATE-compressed data.
 */
__attribute__((optimize("O2")))
__attribute__((hot))
int deflate_decompress(void) {
	bit_buffer = 0;
	buffer_size = 0;

	build_fixed();

	/* read compressed data */
	while (1) {
		/* Read bit */

		int is_final = read_bit();
		int type = read_bits(2);

		switch (type) {
			case 0x00: /* BTYPE=00 Non-compressed blocks */
				uncompressed();
				break;
			case 0x01: /* BYTPE=01 Compressed with fixed Huffman codes */
				inflate(&fixed_lengths, &fixed_dists);
				break;
			case 0x02: /* BTYPE=02 Compression with dynamic Huffman codes */
				decode_huffman();
				break;
			case 0x03:
				return 1;
			default:
				__builtin_unreachable();
				break;
		}

		if (is_final) {
			break;
		}
	}

	return 0;
}

#define GZIP_FLAG_TEXT (1 << 0)
#define GZIP_FLAG_HCRC (1 << 1)
#define GZIP_FLAG_EXTR (1 << 2)
#define GZIP_FLAG_NAME (1 << 3)
#define GZIP_FLAG_COMM (1 << 4)

static unsigned int read_32le(void) {
	unsigned int a, b, c, d;
	a = read_byte();
	b = read_byte();
	c = read_byte();
	d = read_byte();

	return (d << 24) | (c << 16) | (b << 8) | (a << 0);
}

int gzip_decompress(void) {

	/* Read gzip headers */
	if (read_byte() != 0x1F) return 1;
	if (read_byte() != 0x8B) return 1;

	unsigned int cm = read_byte();
	if (cm != 8) return 1;

	unsigned int flags = read_byte();

	/* Read mtime */
	unsigned int mtime = read_32le();
	(void)mtime;

	/* Read extra flags */
	unsigned int xflags = read_byte();
	(void)xflags;

	/* Read and discord OS flag */
	unsigned int os = read_byte();
	(void)os;

	/* Extra bytes */
	if (flags & GZIP_FLAG_EXTR) {
		unsigned short size = read_16le();
		for (unsigned int i = 0; i < size; ++i) read_byte();
	}

	if (flags & GZIP_FLAG_NAME) {
		unsigned int c;
		while ((c = read_byte()) != 0);
	}

	if (flags & GZIP_FLAG_COMM) {
		unsigned int c;
		while ((c = read_byte()) != 0);
	}

	unsigned int crc16 = 0;
	if (flags & GZIP_FLAG_HCRC) {
		crc16 = read_16le();
	}
	(void)crc16;

	int status = deflate_decompress();

	/* Read CRC and decompressed size from end of input */
	unsigned int crc32 = read_32le();
	unsigned int dsize = read_32le();

	(void)crc32;
	(void)dsize;

	return status;
}
