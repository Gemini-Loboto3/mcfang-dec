// quintdec.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "lz.h"
#include "gfx.h"
#include "snes.h"

#define EXPANSION_PTR	0x100000	// expansion area for new graphics
#define GFX_PTR_BASE	0x7d40		// references to all compressed gfx in game

#define IS_COMPRESSOR	1			// compress all png images from a "gfx" folder to an expanded bank of the rom
#define IS_DECOMPRESSOR	0			// dump all gfx as bitmap and binaries to a "bin" folder

#define MCDEC_MODE		IS_COMPRESSOR

int _tmain(int argc, TCHAR *argv[])
{
#if MCDEC_MODE	// change this to turn into either extractor or inserter
	MEM_STREAM str, data;
	GString name;

	if (argc < 3)
	{
		_tprintf(_T("Usage: mccmp <input.sfc> <output.sfc>\n")
			_T("Example: mccmp input.sfc output.sfc\n"));
		return 1;
	}
	if (!FileExists(argv[1]))
	{
		_tprintf(_T("ERROR: Cannot find file %s.\n"), argv[1]);
		return 1;
	}

	// cache rom
	MemStreamOpen(&str, argv[1]);
	MemStreamSeek(&str, GFX_PTR_BASE, SEEK_SET);	// table of gfx data
	u32 ptr_base = EXPANSION_PTR;	// expanded rom space
	// data to write to rom
	MemStreamCreate(&data);
	for (int i = 0; i < 173; i++)
	{
		name.Format(_T("gfx\\%03d.png"), i);
		if (!FileExists(name))
		{
			// skip current pointer
			MemStreamSeek(&str, 3, SEEK_CUR);
			continue;
		}

		_tprintf(_T("Compressing %s...\n"), (LPCTSTR)name);

		Image img;
		img.LoadFromFile(name);

		u8 *dst, *cdst; int size;
		GFX_HEADER head;
		// image 001 and 002 are 2 bpp
		if (i == 1 || i == 2)
			ConvertImage2bpp(img, &head, dst, size);
		// the rest are 4bpp
		else ConvertImage4bpp(img, &head, dst, size);

		// compress tiles and write header data
		size = Compress(cdst, dst, size);
		MemStreamWrite(&data, &head, sizeof(head));
		MemStreamWrite(&data, cdst, size);
		// update pointer table
		u32 p = ToLorom(ptr_base);
		MemStreamWrite(&str, &p, 3);
		// kill temp buffers
		delete[] dst;
		delete[] cdst;
		// seek forward
		ptr_base += size + 3;
	}

	// write assembled compressed data to rom
	MemStreamSeek(&str, EXPANSION_PTR, SEEK_SET);
	MemStreamWrite(&str, data.data, data.size);
	// pad rom to bank size
	for (int i = str.size, si = align(str.size, 0x8000); i < si; i++)
		MemStreamWriteByte(&str, 0);
	// done, flush to output file
	MemStreamFlush(&str, argv[2]);
	// kill memory streams
	MemStreamClose(&str);
	MemStreamClose(&data);
#else
	if (argc < 2)
	{
		_tprintf(_T("Usage: mcdec <input.sfc>\n")
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
#endif

    return 0;
}

