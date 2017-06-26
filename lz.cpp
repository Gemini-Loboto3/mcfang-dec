#include "stdafx.h"

typedef struct tagLzHeader
{
	u8 type;	// 0-1 do nothing (not supposed to happen), 2 compressed
	u8 unk;		// seem to be ignored
	u16 csiz;	// compressed size (header + data, ignored)
	u16 size;	// size of decompressed data (this is used to setup DMA transfers)
} LZ_HEADER;

#define LITERAL	1
#define PAIR	0

class CLzsWrite
{
public:
	CLzsWrite(int src_size)
	{
		this->src_size = src_size;
		out = new u8[src_size * 2];	// allocate enough room for output
		outpos = sizeof(LZ_HEADER);
		bitpos = 0;
	}

	void WriteByte(u8 byte, bool flush = true)
	{
		out[outpos++] = byte;
	}

	void WriteFlg(u8 bit)
	{
		// ran out of bits, seek to next flag byte
		if (bitpos == 0)
		{
			bytepos = outpos++;
			bitpos = 8;
			out[bytepos] = 0;
		}
		// write the flag itself
		out[bytepos] <<= 1;
		if(bit) out[bytepos] |= bit & 1;
		--bitpos;
	}

	void Flush()
	{
		// nothing to write anymore
		if (bitpos == 8) return;
		// adjust bit shifting otherwise
		out[bytepos] <<= bitpos;
	}

	void WriteHeader()
	{
		LZ_HEADER h;
		h.type = 2;
		h.unk = 0;
		h.csiz = outpos + sizeof(LZ_HEADER);
		h.size = src_size;
		memcpy(out, &h, sizeof(h));
	}

	u8 *GetData() { return out; }
	int GetSize() { return outpos; }

private:
	u8 *out;
	int bitpos,		// bit position
		bytepos,	// flag pointer
		outpos,		// output related
		src_size;
};

class CLzsRead
{
public:
	CLzsRead(u8 *data)
	{
		in = data;
		pos = 0;
		flg = 0x8000;
	}

	u8 ReadByte()
	{
		return in[pos++];
	}

	int ReadFlg()
	{
		int ret = 0;
		// check if we need 8 new bits
		if (flg == 0x8000)
			flg = (in[pos++] << 8) | 0x80;
		ret = flg & 0x8000 ? 1 : 0;
		flg <<= 1;
		return ret;
	}

private:
	u8 *in;
	int pos;
	u16 flg;
};

#define LZ_JUMP			0x2000
#define LZ_MINLEN		3
#define LZ_LENGTH		256

// ************************************ //
// DECOMPRESSION						//
// ************************************ //
u32 Decompress(u8 *data, u8 *&dest)
{
	LZ_HEADER *head = (LZ_HEADER*)data;
	CLzsRead r(&data[sizeof(LZ_HEADER)]);

	// allocate enough data for writing
	int size = head->size;
	dest = new u8[size];
	// setup ring buffer
	u8 window[LZ_JUMP];	// fun fact: no gfx uses ever more than 8KB
						// in other words, this buffer is actually useless
	int wpos = 0, opos = 0;

	do
	{
		// literal
		if (r.ReadFlg() == LITERAL)
		{
			u8 b = r.ReadByte();
			// write out
			dest[opos++] = b;
			if (opos >= size) break;
			// fill window
			window[wpos++] = b;
			wpos &= 0x1FFF;
		}
		// compressed
		else
		{
			s16 jmp;
			int len;

			// regular/extended lzs (0 1 jjjjjjjjj jjjjjlll)
			// NOTE: can also trigger end of decompression
			if (r.ReadFlg())
			{
				u8 b0 = r.ReadByte(), b1 = r.ReadByte();
				jmp = ((b0 << 8) | b1) >> 3;
				len = b1 & 7;	// max = 7 (+2)
				// regular lzs
				if (len) len++;
				// 0 1 jjjjjjjj jjjjj000
				else
				{
					// extended lzs
					// 0 1 jjjjjjjj jjjjj000 llllllll
					len = r.ReadByte();	// max = 255 (+1)
					// 0 1 xxxxxxxx xxxxx000 00000000 (end of decompression marker)
					if (len == 0) return size;
				}
			}
			// compact lzs (0 0 l l jjjjjjjj)
			else
			{
				len = r.ReadFlg() * 2 + r.ReadFlg() + 1;	// max = 3 (+2)
				jmp = r.ReadByte() | 0xFF00;				// can only jump back by 255 bytes
			}

			jmp = (jmp + wpos) & 0x1FFF;
			len++;

			// decompress
			for (int j = 0; j < len; j++)
			{
				// write out
				dest[opos++] = window[jmp];
				if (opos >= size) break;
				// fill window
				window[wpos++] = window[jmp++];
				wpos &= 0x1FFF;
				jmp &= 0x1FFF;
			}
		}
	} while (opos < size);	// this is more like a failsafe
							// game quits decompression only via signal

	return size;
}

// ************************************ //
// COMPRESSION							//
// ************************************ //
typedef struct tagLzssData
{
	s16 LzByteStartTable[256];
	s16 LzByteEndTable[256];
	s16 LzNextOffsetTable[LZ_JUMP];
	u32 WindowLen;
	u32 WindowPos;
	u32 InputSize;
	u32 InputPos;
} LZSS_DATA;

static void LzssInit(LZSS_DATA& Data, u8* OutputBuffer, int Size)
{
	memset(Data.LzByteStartTable,	-1, sizeof(Data.LzByteStartTable));
	memset(Data.LzByteEndTable,		-1, sizeof(Data.LzByteEndTable));
	memset(Data.LzNextOffsetTable,	-1, sizeof(Data.LzNextOffsetTable));
	Data.WindowLen = 0;
	Data.WindowPos = 0;
	Data.InputSize = Size;
	Data.InputPos = 0;
}

static void LzssSlideByte(LZSS_DATA& Data, u8* Source, int SourcePos)
{
	u8 InputByte = Source[SourcePos];
	int WindowInsertPos = 0;

	if (Data.WindowLen == LZ_JUMP)	// full
	{
			u8 OldByte = Source[SourcePos - LZ_JUMP];
			Data.LzByteStartTable[OldByte] = Data.LzNextOffsetTable[Data.LzByteStartTable[OldByte]];
			// no more bytes to pull?
			if (Data.LzByteStartTable[OldByte] == -1) Data.LzByteEndTable[OldByte] = -1;	// place and end mark
	}
	WindowInsertPos = Data.WindowPos;

	// load last occurrence of this byte
	int LastOffset = Data.LzByteEndTable[InputByte];
	 // byte is not mentioned?
	if (LastOffset == -1) Data.LzByteStartTable[InputByte] = WindowInsertPos;		// write position as start
	else Data.LzNextOffsetTable[LastOffset] = WindowInsertPos;		// write position as next

	Data.LzByteEndTable[InputByte] = WindowInsertPos;
	Data.LzNextOffsetTable[WindowInsertPos] = -1;

	Data.WindowPos = (Data.WindowPos+1) % LZ_JUMP;
	// full
	if (Data.WindowLen < LZ_JUMP)
		Data.WindowLen++;	// seek forward
}

static void LzssAdvanceBytes(LZSS_DATA& Data, u8* Input, int num)
{
	for (int i = 0; i < num; i++)
		LzssSlideByte(Data,Input,Data.InputPos+i);
	Data.InputPos += num;
}

static bool LzssSearchMatch(LZSS_DATA& Data, u8* Input, int& ReturnPos, int& ReturnSize)
{
	int Offset = Data.LzByteStartTable[Input[Data.InputPos]];	// ersten offset dieses bytes laden
	int SearchOffset;
	int maxchain = 0;
	int maxpos = 0;
	int InputPos = Data.InputPos;
	int MaxInput = Data.InputSize;

	while (Offset != -1)
	{
		// calculate position
		if (Offset == Data.WindowPos)
		{
			Offset = Data.LzNextOffsetTable[Offset];
			continue;
		}

		if (Offset < (int)Data.WindowPos) SearchOffset = InputPos-Data.WindowPos+Offset;
		else SearchOffset = InputPos-Data.WindowLen-Data.WindowPos+Offset;

		int chain = 0;
		while (InputPos+chain < MaxInput && Input[SearchOffset+chain] == Input[InputPos+chain])
		{
			chain++;
			if(chain == LZ_LENGTH) break;
		}

		if (chain > maxchain)
		{
			maxchain = chain;
			maxpos = InputPos-SearchOffset;
			if (maxchain == LZ_LENGTH) break;
		}

		Offset = Data.LzNextOffsetTable[Offset];	// next offset loads these bytes
	}

	ReturnPos = maxpos;
	ReturnSize = maxchain;
	// special compact case
	if (maxchain >= 2 && maxchain <= 5 && maxpos <= 255)
		return true;
	// regular lzs
	if (maxchain < LZ_MINLEN)
		return false;
	
	return true;
}

u32 Compress(u8* &out, u8* in, u32 src_size)
{
	int jmp, len;
	u32 in_pos = 0;

	CLzsWrite w(src_size);
	LZSS_DATA d;

	LzssInit(d, out, src_size);
	for (; d.InputPos < src_size;)
	{
		// search back for data
		if (LzssSearchMatch(d, in, jmp, len))
		{
			w.WriteFlg(PAIR);
			// compact lzs
			if (jmp <= 0xff && len <= 5)
			{
				w.WriteFlg(0);
				w.WriteFlg(((len - 2) & 2) >> 1);
				w.WriteFlg((len - 2) & 1);
				w.WriteByte((-jmp) & 0xff);
			}
			else
			{
				w.WriteFlg(1);
				jmp = -jmp;
				// extended lzs
				if (len > 9)
				{
					u16 pair = (jmp & 0x1fff) << 3;
					w.WriteByte(pair >> 8, false);
					w.WriteByte(pair & 0xff, false);
					w.WriteByte(len - 1);
				}
				// regular lzs
				else
				{
					u16 pair = ((jmp & 0x1fff) << 3) | (len - 2);
					w.WriteByte(pair >> 8, false);
					w.WriteByte(pair & 0xff);
				}
			}
			LzssAdvanceBytes(d, in, len);
		}
		// no match found, write literal
		else
		{
			w.WriteFlg(LITERAL);
			w.WriteByte(in[d.InputPos]);
			LzssAdvanceBytes(d, in, 1);
		}
	}
	// end of compression sequence (01 00000000 00000000 00000000)
	w.WriteFlg(0);
	w.WriteFlg(1);
	w.WriteByte(0);
	w.WriteByte(0);
	w.WriteByte(0);
	// write flag if there are any pending bits
	w.Flush();
	// update header
	w.WriteHeader();

	out = w.GetData();
	return w.GetSize();
}