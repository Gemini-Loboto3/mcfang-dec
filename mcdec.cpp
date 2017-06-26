// quintdec.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "lz.h"
#include "gfx.h"
#include "snes.h"

#define GFX_PTR_BASE	0x7d40		// references to all compressed gfx in game

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

	GString name;
	u8 *r = &rom[GFX_PTR_BASE];
	Image img;
	CreateDirectory(_T("bin"), NULL);
	for (int i = 0; i < 173; i++)
	{
		u32 p = FromLorom(read_u24(r));
		GFX_HEADER *h = (GFX_HEADER*)&rom[p];
		if (p == 0) { r += 3;  continue; }
		fprintf(log, "%03d: %02x %02x %02x\r\n",
			i, h->w, h->h, h->depth);
		int size = Decompress(&rom[p+3], dst);
		name.Format(_T("bin\\%03d.bin"), i);
		FlushFile(name, dst, size);
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
		name.Format(_T("bin\\%03d.bmp"), i);
		img.SaveBitmap(name);

		delete[] dst;
		r += 3;
	}
	fclose(log);

    return 0;
}

