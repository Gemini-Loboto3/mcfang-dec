#pragma once

u32 read_u24(u8 *rom);

// a couple useful functions to convert pointers
u32 ToLorom(u32 ptr);	// PC to lorom
u32 FromLorom(u32 ptr);	// lorom to PC
