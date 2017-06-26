#include "stdafx.h"
#include "snes.h"

u32 read_u24(u8 *rom)
{
	return rom[0] | (rom[1] << 8) | (rom[2] << 16);
}

void write_u24(u8 *rom, u32 w)
{
	rom[0] = w & 0xFF;
	rom[1] = (w & 0xFF00) >> 8;
	rom[2] = (w & 0xFF0000) >> 16;
}

static __inline u32 getlo(u32 addr)
{
	return (addr & 0xff);
}

static __inline u32 gethi(u32 addr)
{
	return ((addr >> 8) & 0x7f) + 0x80;
}

static __inline u32 getbank(u32 addr)
{
	return ((addr >> 15) & 0xff) + 0x80;
}

u32 ToLorom(u32 ptr)
{
	return getlo(ptr) | (gethi(ptr) << 8) | (getbank(ptr) << 16);
}

u32 FromLorom(u32 ptr)
{
	return (ptr & 0x7fff) + ((ptr >> 1) & 0x3f8000);
}
