#ifndef DRAWCON_H_
#define DRAWCON_H_

#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

const uint16_t UCODE_SHADE[] = { 0x2591, 0x2592, 0x2593, 0x2588, 0x2592, 0x2593, 0x2588, 0x2588 };

static HANDLE h_console;
static SMALL_RECT write_rect;
static CHAR_INFO *chi_buffer = NULL;
static COORD coord_buf_size;
static COORD coord_buf_coord;
static uint16_t width, height;

CHAR_INFO* InitDrawBuffer(HANDLE h_con, uint16_t w, uint16_t h)
{
	h_console = h_con;
	width = w;
	height = h;
	
	coord_buf_size.X = w;
	coord_buf_size.Y = h;

	coord_buf_coord.X = 0;
	coord_buf_coord.Y = 0;

	write_rect.Top = 0;
	write_rect.Left = 0;
	write_rect.Bottom = h - 1;
	write_rect.Right = w - 1;

	if (chi_buffer != NULL)
		free(chi_buffer);
	chi_buffer = (CHAR_INFO *)malloc(w * h * sizeof(CHAR_INFO));
	
	return chi_buffer;
}

int DrawCon(uint8_t *scr)
{
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			int bgc = 0, pos0 = j + i * width, pos = (j + i * width) * 3;
			int r = scr[pos], g = scr[pos + 1], b = scr[pos + 2];
			int intensity = (299 * r + 587 * g + 114 * b) / 1000;
			int sq = (int)sqrtf((float)(r * r + g * g + b * b));
			sq = sq > 0 ? sq : 256;
			r = (r << 8) / sq;
			g = (g << 8) / sq;
			b = (b << 8) / sq;

			bgc = (intensity >> 7 << 3) + (r >> 7 << 2) + (g >> 7 << 1) + (b >> 7);

			chi_buffer[pos0].Attributes = bgc;
			chi_buffer[pos0].Char.UnicodeChar = UCODE_SHADE[(int)roundf((float)intensity / 256 * 8)];
		}
	}

	WriteConsoleOutputW(
		h_console,
		chi_buffer,
		coord_buf_size,
		coord_buf_coord,
		&write_rect
	);

	return 0;
}

#endif