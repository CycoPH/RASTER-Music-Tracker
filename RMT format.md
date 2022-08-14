# RMT 1.x file format

## Binary Block
The RMT file format is broken down into various binary blocks. Each block starts with a 4-6 byte binary header. The header indicates where in memory the data is loaded.

|Offset|Type|Description
|--|--|---
| 00 | WORD | 0xFFFF binary header
| 02 | WORD | Load address start
| 04 | WORD | End address

i.e. FF FF 00 40 39 51 gives a load address of $4000, last byte will be located at $5139

## Header structure

|Offset|Type|Description
|--|--|---
| 00 | WORD | Header string 'RMT4' or 'RMT8'
| 04 | BYTE | Track length ($00 means 256)
| 05 | BYTE | Song speed
| 06 | BYTE | Instrument speed (1-4)
| 07 | BYTE | Format version number ($01 for player routine 1.x compatible format)
| 08 | WORD | Pointer to instruments table (ptrInstruments)
| 0a | WORD | Pointer to low bytes of tracks table (ptrTracksLow)
| 0c | WORD | Pointer to high bytes of tracks table (ptrTracksHigh)
| 0e | WORD | Pointer to track lines (SONG) (ptrSong)

## Instruments
Instrument data normally follows the RMT header directly. Each entry is a pointer to the instrument location in the binary block.
i.e. numInstruments = (ptrTracksStart - ptrInstruments) / 2

Each instrument header is 12 bytes long.

|Offset | Type | Description
|---|---|---
| 00 | BYTE	| Note table length (pointer to end of table of notes)
| 01 | BYTE | Note table goto (pointer to loop of table of notes)
| 02 | BYTE	| Envelope length (pointer to end of envelope) [Min 1]
| 03 | BYTE	| Envelope goto (pointer to loop of envelope)
| 04 | BYTE	| Bits: 0-5 = Note speed [0-63]<br>Bit: 6 = Type of table : notes (0) or frequencies (1)<br> Bit: 7 = Set or add Note/Frequency : Set (0) or Add (1)
| 05 | BYTE	| AUDCTL Pokey values
| 06 | BYTE	| Volume fadeout speed
| 07 | BYTE	| Minimum volume (bit 4-7), lower nibble is not used
| 08 | BYTE	| Delay ($00 for no vibrato & no frequency shift)
| 09 | BYTE	| Vibrato
| 0a | BYTE	| frequency shift
| 0b | BYTE	| unused
| 0c | BYTE	| Array of notes or frequency (according to the table type bit)
| ?  | 		| Envelope

### TABLE OF NOTES
	BYTE[]	note or frequency (according to the table type)

### ENVELOPE
Each envelope entry (there is at least one) has the following format.

|Offset | Type | Description
|---|---|---
| 0 | BYTE | Volume (bit 0-3 left channel) (bits 4-7 right channel (in RMT4 it's the same as bits 0-3))
| 1 | BYTE | Bit 0: Portamento<br>Bits 1-3: Distortion<br>Bits 4-6: Command<br>Bit 7: Filter
| 2 | BYTE | XY parameter to the command

Left channel volume is specified in the lower nibble. If the song is mono then the right volume is set to the left volume, otherwise the high nibble is the right volume.

Distortion is an even number from this table:

	0 = white noise. (AUDC $0v, Poly5+17/9)
	2 = square-ish tones. (AUDC $2v, Poly5)
	4 = no note table yet, Pure Table by default. (AUDC $4v, Poly4+5)
	6 = 16-Bit tones in valid channels (Distortion A by default)
	8 = white noise. (AUDC $8v, Poly17/9)
	A/10 = pure tones. Special mode: CH1+CH3 1.79mhz + AUTOFILTER = Sawtooth (AUDC $Av)
	C/12 = buzzy, gritty bass tones. (AUDC $Cv, Poly4)
	E/14 = ???

Command is one of the following 8 values:

	0 = Play BASE_NOTE + $XY semitones
	1 = Play frequency $XY
	2 = Play BASE_NOTE + frequency $XY
	3 = Set BASE_NOTE += $XY semitones. Play BASE_NOTE
	4 = Set FREQUENCY SHIFT += frequency $XY. Play BASE_NOTE
	5 = Set portamento speed $X, step $Y. Play BASE_NOTE
	6 = Set FILTER_SHFRQ += $XY. $0Y = BASS16 Distortion. $FF/$01 = Sawtooth inversion (Distortion A)
	7 = Set instrument AUDCTL. $FF = VOLUME ONLY mode. $FE/$FD = enable/disable Two-Tone Filter."

## TRACK
|Type | Description
|---|---
| `data`=<br/>BYTE | Bit 0-5: Note [data & $3F] = `note`<br/>Bit 6-7: volume(LO) or pause(1-3 beats) or special = `high2`
| `note`=$00 - \$3C<br/> 0 - 60 | Next byte is volume and instrument info<br/>Note = `note`<br/>Bit 0-1: volume(HI)<br/>Bit 2-7 Instrument number<br/>Volume = `high2` + Bit 0-1
| | If `data` is $3D / 61 (volume only change) then next byte has only volume info
| BYTE | Bit 0-1: volume(HI)<br/>Volume only
| `note`=62 | Bit 6-7: `pause`<br/>If `pause` is 1-3: Pause for 1-3 beats.<br/>If `pause` is 0: Next byte pauses 1-255 beats, or if 0 then indicates `The End`
| `data`=63| This is a speed change, the next byte is the speed value.<br/>NOTE: Speed changes are always stored before note data.
| `data`=191 | This is a go line command.<br/>Next byte is the song line # to continue from.
| `data`=255 | This is the end of the track.

INSTRUMENTS TABLE
=================
	WORD	ptr_instr0
	WORD	ptr_instr1
	WORD	ptr_instr3
	...

TRACKS TABLE (LO)
=================
	BYTE	lowbyte_of_ptr_track0
	BYTE	lowbyte_of_ptr_track1
	BYTE	lowbyte_of_ptr_track2
	...

TRACKS TABLE (HI)
=================
	BYTE	highbyte_of_ptr_track0
	BYTE	highbyte_of_ptr_track1
	BYTE	highbyte_of_ptr_track2
	...

TRACK LIST struct (SONG)
========================
	BYTE	tracknumL1,tracknumL2,tracknumL3,tracknumL4,[tracknumR1,..,tracknumR4]
	BYTE	tracknumL1,tracknumL2,tracknumL3,tracknumL4,[tracknumR1,..,tracknumR4]
	BYTE	tracknumL1,tracknumL2,tracknumL3,tracknumL4,[tracknumR1,..,tracknumR4]
	...

if tracknum is FF, then empty track is used

if tracknumL1 is FE, then gotoline(BYTE)=tracknumL2, goto_pointer(WORD)=(tracknumL3,4)
	Note: gotoline(BYTE) is not used in player (but tracker uses it)

