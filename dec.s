decompress:
	REP #$30 ; '0'
	LDX #0
	STX D,dec_winpos
	LDA D,dec_src
	TAY
	STZ D,dec_src
	INY	 ; *src++ (skip type)
	BNE @@bankskip_0
	LDY #$8000
	INC D,dec_src+2

@@bankskip_0:
	INY	 ; *src++
	BNE @@bankskip_1
	LDY #$8000
	INC D,dec_src+2

@@bankskip_1:
	INY	 ; *src++
	BNE @@bankskip_2
	LDY #$8000
	INC D,dec_src+2

@@bankskip_2:
	INY	 ; *src++
	BNE @@bankskip_3
	LDY #$8000
	INC D,dec_src+2

@@bankskip_3:
	SEP #$20 ; ' '
	LDA [D,dec_src],Y
	INY	 ; *src++
	BNE @@bankskip_4
	LDY #$8000
	INC D,dec_src+2

@@bankskip_4:
	XBA
	LDA [D,dec_src],Y
	INY	 ; *src++
	BNE @@bankskip_5
	LDY #$8000
	INC D,dec_src+2

@@bankskip_5:
	XBA
	REP #$20 ; ' '
	PHA	 ; push data read from header
	LDA #$8000
	STA D,dec_flag  ; = 0x8000
	STZ D,dec_dstpos    ; = 0
	JMP @@read_flag
; ---------------------------------------------------------------------------

@@flag_sub0:
	STZ D,dec_lzslen    ; len = 0
	LDA D,dec_flag
	CMP #$8000
	BNE @@skip_flag2
	LDA [D,dec_src],Y
	INY
	BNE @@bankskip_11
	LDY #$8000
	INC D,dec_src+2

@@bankskip_11:
	XBA
	AND #$FF00
	ORA #$80 ; 'Ç'
	STA D,dec_flag

@@skip_flag2:
	ASL D,dec_flag  ; flag <<= 1
	ROL D,dec_lzslen
	LDA D,dec_flag
	CMP #$8000
	BNE @@skip_flag3
	LDA [D,dec_src],Y
	INY
	BNE @@bankskip_12
	LDY #$8000
	INC D,dec_src+2

@@bankskip_12:
	XBA
	AND #$FF00
	ORA #$80 ; 'Ç'
	STA D,dec_flag

@@skip_flag3:
	ASL D,dec_flag
	ROL D,dec_lzslen
	LDA [D,dec_src],Y
	INY
	BNE @@bankskip_13
	LDY #$8000
	INC D,dec_src+2

@@bankskip_13:
	AND #$FF
	ORA #$FF00
	CLC
	ADC D,dec_winpos    ; jmp + winpos
	AND #$1FFF
	STA D,dec_lzjump

@@dec_loop:
	INC D,dec_lzslen    ; len++

@@dec_loop1:
	INC D,dec_lzslen    ; len++
	PHY
	LDY D,dec_dstpos

@@copy:
	SEP #$20 ; ' '
	LDX D,dec_lzjump
	LDA $7E8000,X
	LDX D,dec_winpos
	STA $7E8000,X
	STA [D,dec_dst],Y
	INY
	REP #$20 ; ' '
	TXA
	INC
	AND #$1FFF
	STA D,dec_winpos
	LDA D,dec_lzjump
	INC
	AND #$1FFF
	STA D,dec_lzjump
	DEC D,dec_lzslen
	BNE @@copy
	STY D,dec_dstpos
	PLY

@@read_flag:
	LDA D,dec_flag
	CMP #$8000  ; when == 0x8000 we need a new flag read
	BNE @@skip_flag
	LDA [D,dec_src],Y   ; *src++
	INY
	BNE @@bankskip_6
	LDY #$8000
	INC D,dec_src+2

@@bankskip_6:
	XBA
	AND #$FF00  ; isolate first byte
	ORA #$80 ; 'Ç'  ; arrange flag to be = (byte << 8) | 0x80
	STA D,dec_flag

@@skip_flag:
	ASL D,dec_flag  ; flag<<=1
	BCC @@flg_comp  ; jump if(!flag & 0x80) [compressed]
	SEP #$20 ; ' '
	LDA [D,dec_src],Y   ; A = *src++
	INY
	BNE @@bankskip_7
	LDY #$8000
	INC D,dec_src+2

@@bankskip_7:
	PHY
	LDY D,dec_dstpos
	STA [D,dec_dst],Y
	INY
	STY D,dec_dstpos
	PLY
	LDX D,dec_winpos
	STA $7E8000,X   ; *dst = A
	INX
	REP #$20 ; ' '
	TXA
	AND #$1FFF
	STA D,dec_winpos
	BRA @@read_flag
; ---------------------------------------------------------------------------

@@flg_comp:
	LDA D,dec_flag  ; try and get another bit from flag
	CMP #$8000  ; read from source if there are no more bits left
	BNE @@skip_flag1    ; < --
	LDA [D,dec_src],Y   ; --
	INY	 ; --
	BNE @@bankskip_8    ; --
	LDY #$8000  ; --
	INC D,dec_src+2

@@bankskip_8:
	XBA
	AND #$FF00
	ORA #$80 ; 'Ç'
	STA D,dec_flag  ; -- >

@@skip_flag1:
	ASL D,dec_flag
	BCS @@flag_sub1 ; jump if bit is 1
	JMP @@flag_sub0 ; jump if bit is 0
; ---------------------------------------------------------------------------

@@flag_sub1:
	SEP #$20 ; ' '
	LDA [D,dec_src],Y   ; A = *src++ | (*src++ << 8)
	INY
	BNE @@bankskip_9
	LDY #$8000
	INC D,dec_src+2

@@bankskip_9:
	XBA
	LDA [D,dec_src],Y
	INY
	BNE @@bankskip_10
	LDY #$8000
	INC D,dec_src+2

@@bankskip_10:
	XBA
	REP #$20 ; ' '
	XBA	 ; size = A << 3
	PHA
	SEC
	ROR
	SEC
	ROR
	SEC
	ROR
	CLC
	ADC D,dec_winpos
	AND #$1FFF
	STA D,dec_lzjump
	PLA
	AND #111b   ; size = A & 7
	STA D,dec_lzslen
	BEQ @@len_null
	JMP @@dec_loop
; ---------------------------------------------------------------------------

@@len_null:
	LDA [D,dec_src],Y
	INY
	BNE @@bankskip_14
	LDY #$8000
	INC D,dec_src+2

@@bankskip_14:
	AND #$FF
	STA D,dec_lzslen
	BEQ @@end
	JMP @@dec_loop1
; ---------------------------------------------------------------------------

@@end:
	PLA
	RTS
