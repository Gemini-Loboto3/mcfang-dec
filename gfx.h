#pragma once

// graphics conversion

typedef struct tagGfxHeader
{
	u8 w, h;	// width / height in tiles
	u8 depth;	// either 2 or 4
} GFX_HEADER;

// from SNES to bitmap
void MakeImage2bpp(Image &img, GFX_HEADER *head, u8 *data);
void MakeImage4bpp(Image &img, GFX_HEADER *head, u8 *data);
// from bitmap to SNES
void ConvertImage2bpp(Image &img, GFX_HEADER *head, u8* &data, int &size);
void ConvertImage4bpp(Image &img, GFX_HEADER *head, u8* &data, int &size);
