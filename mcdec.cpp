// quintdec.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "lz.h"
#include "gfx.h"
#include "snes.h"

#define GFX_PTR_BASE	0x7d40		// references to all compressed gfx in game
#define TILEMAP_PTR_BASE	0x0DDE00	// references to all compressed tilemaps in game

int _tmain(int argc, TCHAR *argv[])
{
	if (argc < 2)
	{
		_tprintf(_T("Usage: mcdec <input>\n")
			_T("Example: mdec input.sfc\n"));
		return 1;
	}
	if (!FileExists(argv[1]))
	{
		_tprintf(_T("ERROR: Cannot find file %s.\n"), argv[1]);
		return 1;
	}

	FILE *log = _tfopen(_T("log.txt"), _T("wb+"));
	CBufferFile file(argv[1]);
	u8 *rom = (u8*)file.GetBuffer(),
		*dst;

	GString gfx_name;
	u8 *r = &rom[GFX_PTR_BASE];
	Image img;
	CreateDirectory(_T("bin"), NULL);
	for (int i = 0; i < 173; i++)
	{
		u32 p = FromLorom(read_u24(r));
		GFX_HEADER *h = (GFX_HEADER*)&rom[p];
		if (p == 0) { r += 3;  continue; }
		fprintf(log, "GRAPHIC %03d | PTR: 0x%06X | SIZE: %03d*%03d | BPP: %1x\r\n",
			i, p, h->w * 8, h->h * 8, h->depth);
		int size = Decompress(&rom[p+3], dst);
		gfx_name.Format(_T("bin\\%03d.bin"), i);
		FlushFile(gfx_name, dst, size);
		// convert from either 2bpp or 4bpp
		switch (h->depth)
		{
		case 2:
			MakeImage2bpp(img, h, dst);
			break;
		case 4:
			MakeImage4bpp(img, h, dst);
			break;
		}
		gfx_name.Format(_T("bin\\%03d.bmp"), i);
		img.SaveBitmap(gfx_name);

		delete[] dst;
		r += 3;
	}

	GString tilemap_name;
	u8* r1 = &rom[TILEMAP_PTR_BASE];
	CreateDirectory(_T("tilemap"), NULL);
	for (int i = 0; i < 8; i++)
	{
		u32 p = FromLorom(read_u24(r1));
		GFX_HEADER* h = (GFX_HEADER*)&rom[p];
		if (p == 0) { r1 += 3;  continue; }
		fprintf(log, "TILEMAP %03d | PTR: 0x%06X | SIZE: %03d*%03d\r\n",
			i, p, h->w, h->h);
		int size = Decompress(&rom[p + 2], dst);
		tilemap_name.Format(_T("tilemap\\%03d.bin"), i);
		FlushFile(tilemap_name, dst, size);

		delete[] dst;
		r1 += 3;
	}

	fclose(log);

    return 0;
}

