-----------------------------------------------
       SPIKE MCFANG DE/COMPRESSOR
	       MADE BY GEMINI (2017)
-----------------------------------------------

This is a program for extracting or reinserting compressed
graphics found in "The Twisted Tales of Spike McFang" (US).

It comes in the form of C++ sources for documenting how the
compression works, all packed as a Visual Studio 2015
project. The compression is nothing more than a standard
LZSS with a couple extra encodings for special cases, most
details can be figured from comments in the code. This also
includes a "dec.s" file with the original SNES code used
for decompression in game.

In order to recompile the sources you are going to need a
library of mine called glib (not to be confused with THAT
glib), which can be found here:
https://github.com/Gemini-Loboto3/glib_2015

-----------------------------------------------
             Decompressor (mcdec)
-----------------------------------------------
Usage: mcdec <input>
Example: mdec input.sfc

The decompressor will create a "bin" folder containing a
number of bitmaps and binary dumps of decompressed data.

-----------------------------------------------
              Compressor (mccmp)
-----------------------------------------------
Usage: mccmp <input> <output>
Example: mccmp input.sfc output.sfc

The compressor will look for png images inside a "gfx"
folder, which is provided with edited graphics examples.
Supported png files must use 16 colors and no transparent
indices.

You can specify an output rom to be the same as the input
file.

All newly compressed graphics will be pushed into an extra
bank of the rom, so beware when you apply other expansions.

NOTE: the compression algorithm isn't exactly optimized for
size, as it was originally planned to be a dirty and quick
hack that would reinsert faux compressed data. As it stands
it does a decent job, but it can be improved for minimized
size if you're willing to look into faulty code within the
LZSS jump/length pair generation. Still, you probably won't
need this due to all the room provided by rom expansion.

-----------------------------------------------
                 SPECIAL THANKS
-----------------------------------------------
Kingcom for providing the LZSS match code.