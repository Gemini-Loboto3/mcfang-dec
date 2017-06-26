// quintdec.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "lz.h"
#include "gfx.h"
#include "snes.h"

#define EXPANSION_PTR	0x100000	// expansion area for new graphics
#define GFX_PTR_BASE	0x7d40		// references to all compressed gfx in game

int _tmain(int argc, TCHAR *argv[])
{
	MEM_STREAM str, data;
	GString name;

	if (argc < 3)
	{
		_tprintf(_T("Usage: mccmp <input> <output>\n")
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

    return 0;
}

