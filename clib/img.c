#if HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <getopt.h>

#include "qrencode.h"

#define INCHES_PER_METER (100.0/2.54)

typedef struct {
	int size;
	int margin;
	int dpi;
	unsigned int fg_color[4];
	unsigned int bg_color[4];
} imgPro;

int color_set(unsigned int color[4], const char *value)
{
	int len = strlen(value);
	int count;
	if(len == 6) {
		count = sscanf(value, "%02x%02x%02x%n", &color[0], &color[1], &color[2], &len);
		if(count < 3 || len != 6) {
			return -1;
		}
		color[3] = 255;
	} else if(len == 8) {
		count = sscanf(value, "%02x%02x%02x%02x%n", &color[0], &color[1], &color[2], &color[3], &len);
		if(count < 4 || len != 8) {
			return -1;
		}
	} else {
		return -1;
	}
	return 0;
}

void init(imgPro *pro, int size, int margin, int dpi, const char *fg_val, const char *bg_val)
{
	pro->size = size;
	pro->margin = margin;
	pro->dpi = dpi;

	if (fg_val)
		color_set(pro->fg_color, fg_val);
	else {
		pro->fg_color[0] = 0;
		pro->fg_color[1] = 0;
		pro->fg_color[2] = 0;
		pro->fg_color[3] = 255;
	}

	if(bg_val)
		color_set(pro->bg_color, bg_val);
	else {
		pro->bg_color[0] = 255;
		pro->bg_color[1] = 255;
		pro->bg_color[2] = 255;
		pro->bg_color[3] = 255;
	}
}

int save(imgPro *pro, QRcode *qrcode, const char *outfile)
{
	static FILE *fp; // avoid clobbering by setjmp.
	png_structp png_ptr;
	png_infop info_ptr;
	png_colorp palette;
	png_byte alpha_values[2];
	unsigned char *row, *p, *q;
	int x, y, xx, yy, bit;
	int realwidth;

	realwidth = (qrcode->width + pro->margin * 2) * pro->size;
	row = (unsigned char *)malloc((realwidth + 7) / 8);
	if(row == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return -1;
	}

	if(outfile[0] == '-' && outfile[1] == '\0') {
		fp = stdout;
	} else {
		fp = fopen(outfile, "wb");
		if(fp == NULL) {
			fprintf(stderr, "Failed to create file: %s\n", outfile);
			perror(NULL);
			return -1;
		}
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG writer.\n");
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG write.\n");
		return -1;
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fprintf(stderr, "Failed to write PNG image.\n");
		return -1;
	}

	palette = (png_colorp) malloc(sizeof(png_color) * 2);
	palette[0].red   = pro->fg_color[0];
	palette[0].green = pro->fg_color[1];
	palette[0].blue  = pro->fg_color[2];
	palette[1].red   = pro->bg_color[0];
	palette[1].green = pro->bg_color[1];
	palette[1].blue  = pro->bg_color[2];
	alpha_values[0] = pro->fg_color[3];
	alpha_values[1] = pro->bg_color[3];
	png_set_PLTE(png_ptr, info_ptr, palette, 2);
	png_set_tRNS(png_ptr, info_ptr, alpha_values, 2, NULL);

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr,
			realwidth, realwidth,
			1,
			PNG_COLOR_TYPE_PALETTE,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
	png_set_pHYs(png_ptr, info_ptr,
			pro->dpi * INCHES_PER_METER,
			pro->dpi * INCHES_PER_METER,
			PNG_RESOLUTION_METER);
	png_write_info(png_ptr, info_ptr);

	/* top margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<pro->margin * pro->size; y++) {
		png_write_row(png_ptr, row);
	}

	/* data */
	p = qrcode->data;
	for(y=0; y<qrcode->width; y++) {
		bit = 7;
		memset(row, 0xff, (realwidth + 7) / 8);
		q = row;
		q += pro->margin * pro->size / 8;
		bit = 7 - (pro->margin * pro->size % 8);
		for(x=0; x<qrcode->width; x++) {
			for(xx=0; xx<pro->size; xx++) {
				*q ^= (*p & 1) << bit;
				bit--;
				if(bit < 0) {
					q++;
					bit = 7;
				}
			}
			p++;
		}
		for(yy=0; yy<pro->size; yy++) {
			png_write_row(png_ptr, row);
		}
	}
	/* bottom margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<pro->margin * pro->size; y++) {
		png_write_row(png_ptr, row);
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
	free(row);
	free(palette);

	return 0;
}
