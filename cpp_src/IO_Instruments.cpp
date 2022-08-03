#include "stdafx.h"
#include "resource.h"
#include <fstream>
using namespace std;

#include "Atari6502.h"
#include "IOHelpers.h"

#include "Instruments.h"

#include "global.h"

#include "GuiHelpers.h"


int CInstruments::SaveAll(ofstream& ou, int iotype)
{
	for (int i = 0; i < INSTRSNUM; i++)
	{
		if (iotype == IOINSTR_TXT && !CalculateNotEmpty(i)) continue; //to TXT only non-empty instruments
		SaveInstrument(i, ou, iotype);	//,IOINSTR_RMW);
	}
	return 1;
}

int CInstruments::LoadAll(ifstream& in, int iotype)
{
	for (int i = 0; i < INSTRSNUM; i++)
	{
		LoadInstrument(i, in, iotype);	//IOINSTR_RMW);
	}

	return 1;
}

int CInstruments::SaveInstrument(int instr, ofstream& ou, int iotype)
{
	TInstrument& ai = m_instr[instr];

	int j, k;

	switch (iotype)
	{
		case IOINSTR_RTI:
		{
			//RTI file
			static char head[4] = "RTI";
			head[3] = 1;			//type 1
			ou.write(head, 4);	//4 bytes header RTI1 (binary 1)
			ou.write((const char*)ai.name, sizeof(ai.name)); //name 32 byte + 33 is a binary zero terminating string
			unsigned char ibf[MAXATAINSTRLEN];
			BYTE len = InstrToAta(instr, ibf, MAXATAINSTRLEN);
			ou.write((char*)&len, sizeof(len));				//instrument length in Atari bytes
			if (len > 0) ou.write((const char*)&ibf, len);	//instrument data
		}
		break;

		case IOINSTR_RMW:
			//instrument name
			ou.write((char*)ai.name, sizeof(ai.name));

			char bfpar[PARCOUNT], bfenv[ENVCOLS][ENVROWS], bftab[TABLEN];
			//
			for (j = 0; j < PARCOUNT; j++) bfpar[j] = ai.parameters[j];
			ou.write(bfpar, sizeof(bfpar));
			//
			for (k = 0; k < ENVROWS; k++)
			{
				for (j = 0; j < ENVCOLS; j++)
					bfenv[j][k] = ai.envelope[j][k];
			}
			ou.write((char*)bfenv, sizeof(bfenv));
			//
			for (j = 0; j < TABLEN; j++) bftab[j] = ai.noteTable[j];
			ou.write(bftab, sizeof(bftab));
			//
			//+editing options:
			ou.write((char*)&ai.activeEditSection, sizeof(ai.activeEditSection));
			ou.write((char*)&ai.editNameCursorPos, sizeof(ai.editNameCursorPos));
			ou.write((char*)&ai.editParameterNr, sizeof(ai.editParameterNr));
			ou.write((char*)&ai.editEnvelopeX, sizeof(ai.editEnvelopeX));
			ou.write((char*)&ai.editEnvelopeY, sizeof(ai.editEnvelopeY));
			ou.write((char*)&ai.editNoteTableCursorPos, sizeof(ai.editNoteTableCursorPos));
			//octaves and volumes
			ou.write((char*)&ai.octave, sizeof(ai.octave));
			ou.write((char*)&ai.volume, sizeof(ai.volume));
			break;

		case IOINSTR_TXT:
			//TXT file
			CString s, nambf;
			nambf = ai.name;
			nambf.TrimRight();
			s.Format("[INSTRUMENT]\n%02X: %s\n", instr, (LPCTSTR)nambf);
			ou << (LPCTSTR)s;
			//instrument parameters
			for (j = 0; j < NUMBER_OF_PARAMS; j++)
			{
				s.Format("%s %X\n", shpar[j].name, ai.parameters[j] + shpar[j].displayOffset);
				ou << (LPCTSTR)s;
			}
			//table
			ou << "TABLE: ";
			for (j = 0; j <= ai.parameters[PAR_TBL_LENGTH]; j++)
			{
				s.Format("%02X ", ai.noteTable[j]);
				ou << (LPCTSTR)s;
			}
			ou << endl;
			//envelope
			for (k = 0; k < ENVROWS; k++)
			{
				char bf[ENVCOLS + 1];
				for (j = 0; j <= ai.parameters[PAR_ENV_LENGTH]; j++)
				{
					bf[j] = CharL4(ai.envelope[j][k]);
				}
				bf[ai.parameters[PAR_ENV_LENGTH] + 1] = 0; //buffer termination
				s.Format("%s %s\n", shenv[k].name, bf);
				ou << (LPCTSTR)s;
			}
			ou << "\n"; //gap
			break;
	}
	return 1;
}

int CInstruments::LoadInstrument(int instr, ifstream& in, int iotype)
{
	switch (iotype)
	{
		case IOINSTR_RTI:
		{
			//RTI
			if (instr < 0 || instr >= INSTRSNUM) return 0;
			ClearInstrument(instr);	//it will first delete it before it reads
			TInstrument& ai = m_instr[instr];
			char head[4];
			in.read(head, 4);	//4 bytes header
			if (strncmp(head, "RTI", 3) != 0) return 0;		//if there is no RTI header
			int version = head[3];
			if (version >= 2) return 0;					//it's version 2 and more (only 0 and 1 are supported)

			in.read((char*)ai.name, sizeof(ai.name));	//name 32 bytes + 33rd byte terminating zero

			BYTE len;
			in.read((char*)&len, sizeof(len));			//instrument length in Atari bytes
			if (len > 0)
			{
				unsigned char ibf[MAXATAINSTRLEN];
				in.read((char*)&ibf, len);
				BOOL r;
				if (version == 0)
					r = AtaV0ToInstr(ibf, instr);
				else
					r = AtaToInstr(ibf, instr);
				WasModified(instr);	//writes to Atari ram
				if (!r) return 0; //if there was some problem with the instrument, return 0
			}
		}
		break;

		case IOINSTR_RMW:
		{
			//RMW
			if (instr < 0 || instr >= INSTRSNUM) return 0;
			ClearInstrument(instr);	//it will first delete it before it reads
			TInstrument& ai = m_instr[instr];
			//instrument name
			in.read((char*)ai.name, sizeof(ai.name));

			char bfpar[PARCOUNT], bfenv[ENVCOLS][ENVROWS], bftab[TABLEN];
			int j, k;
			//
			in.read(bfpar, sizeof(bfpar));
			for (j = 0; j < PARCOUNT; j++) ai.parameters[j] = bfpar[j];
			//
			in.read((char*)bfenv, sizeof(bfenv));
			for (j = 0; j < ENVCOLS; j++)
			{
				for (k = 0; k < ENVROWS; k++)
					ai.envelope[j][k] = bfenv[j][k];
			}
			//
			in.read((char*)bftab, sizeof(bftab));
			for (j = 0; j < TABLEN; j++) ai.noteTable[j] = bftab[j];
			//
			WasModified(instr);	//writes to Atari mem
			//
			//+editing options:
			in.read((char*)&ai.activeEditSection, sizeof(ai.activeEditSection));
			in.read((char*)&ai.editNameCursorPos, sizeof(ai.editNameCursorPos));
			in.read((char*)&ai.editParameterNr, sizeof(ai.editParameterNr));
			in.read((char*)&ai.editEnvelopeX, sizeof(ai.editEnvelopeX));
			in.read((char*)&ai.editEnvelopeY, sizeof(ai.editEnvelopeY));
			in.read((char*)&ai.editNoteTableCursorPos, sizeof(ai.editNoteTableCursorPos));
			//octaves and volumes
			in.read((char*)&ai.octave, sizeof(ai.octave));
			in.read((char*)&ai.volume, sizeof(ai.volume));
		}
		break;

		case IOINSTR_TXT:
		{
			char a;
			char b;
			char line[1025];
			in.getline(line, 1024); //first row of the instrument
			int iins = Hexstr(line, 2);

			if (instr == -1) instr = iins; //takes over the instrument number

			if (instr < 0 || instr >= INSTRSNUM)
			{
				NextSegment(in);
				return 1;
			}

			ClearInstrument(instr);	//it will first delete it before it reads
			TInstrument& ai = m_instr[instr];

			char* value = line + 4;
			Trimstr(value);
			memset(ai.name, ' ', INSTRUMENT_NAME_MAX_LEN);
			int lname = INSTRUMENT_NAME_MAX_LEN;
			if (strlen(value) <= INSTRUMENT_NAME_MAX_LEN) lname = strlen(value);
			strncpy(ai.name, value, lname);

			int v, j, k, vlen;
			while (!in.eof())
			{
				in.read((char*)&b, 1);
				if (b == '[') goto InstrEnd;	//end of instrument (beginning of something else)
				line[0] = b;
				in.getline(line + 1, 1024);

				value = strstr(line, ": ");
				if (value)
				{
					value[1] = 0;	//close the gap by closing
					value += 2;	//the first character after the space
				}
				else
					continue;

				for (j = 0; j < NUMBER_OF_PARAMS; j++)
				{
					if (strcmp(line, shpar[j].name) == 0)
					{
						v = Hexstr(value, 2) - shpar[j].displayOffset;
						if (v < 0) goto NextInstrLine;
						v &= shpar[j].parameterAND;
						if (v > shpar[j].maxParameterValue) v = 0;
						ai.parameters[shpar[j].paramIndex] = v;
						goto NextInstrLine;
					}
				}
				if (strcmp(line, "TABLE:") == 0)
				{
					Trimstr(value);
					vlen = strlen(value);
					for (j = 0; j < vlen; j += 3)
					{
						v = Hexstr(value + j, 2);
						if (v < 0) goto NextInstrLine;
						ai.noteTable[j / 3] = v;
					}
					goto NextInstrLine;
				}

				for (j = 0; j < ENVROWS; j++)
				{
					if (strcmp(line, shenv[j].name) == 0)
					{
						for (k = 0; (a = value[k]) && k < ENVCOLS; k++)
						{
							v = Hexstr(&a, 1);
							if (v < 0) goto NextInstrLine;
							v &= shenv[j].pand;
							ai.envelope[k][j] = v;
						}
						goto NextInstrLine;
					}
				}
			NextInstrLine: {}
			}
		}
	InstrEnd:
		WasModified(instr);	//write to Atari mem
		break;

	}

	return 1;
}



BYTE CInstruments::InstrToAta(int instr, unsigned char* ata, int max)
{
	TInstrument& ai = m_instr[instr];
	int i, j;
	int* par = ai.parameters;

	/*
	  0 IDXTABLEEND
	  1 IDXTABLEGO
	  2 IDXENVEND
	  3 IDXENVGO
	  4 TABTYPEMODESPEED
	  5 AUDCTL
	  6 VSLIDE
	  7 VMIN
	  8 EFFDELAY
	  9 EFVIBRATO
	 10 FSHIFT
	 11 0 (unused)
	*/

	const int INSTRPAR = 12;			//12th byte starts the table

	int tablelast = par[PAR_TBL_LENGTH] + INSTRPAR;
	ata[0] = tablelast;
	ata[1] = par[PAR_TBL_GOTO] + INSTRPAR;
	ata[2] = par[PAR_ENV_LENGTH] * 3 + tablelast + 1;	//behind the table is the envelope
	ata[3] = par[PAR_ENV_GOTO] * 3 + tablelast + 1;
	//
	ata[4] = (par[PAR_TBL_TYPE] << 7)
		| (par[PAR_TBL_MODE] << 6)
		| (par[PAR_TBL_SPEED]);
	//
	ata[5] = par[PAR_AUDCTL_15KHZ]
		| (par[PAR_AUDCTL_HPF_CH2] << 1)
		| (par[PAR_AUDCTL_HPF_CH1] << 2)
		| (par[PAR_AUDCTL_JOIN_3_4] << 3)
		| (par[PAR_AUDCTL_JOIN_1_2] << 4)
		| (par[PAR_AUDCTL_179_CH3] << 5)
		| (par[PAR_AUDCTL_179_CH1] << 6)
		| (par[PAR_AUDCTL_POLY9] << 7);
	ata[6] = par[PAR_VOL_FADEOUT];
	ata[7] = par[PAR_VOL_MIN] << 4;
	ata[8] = par[PAR_DELAY];
	ata[9] = par[PAR_VIBRATO] & 0x03;
	ata[10] = par[PAR_FREQ_SHIFT];
	ata[11] = 0; //unused, for now

	//the entire table length gets the data copied
	for (i = 0; i <= par[PAR_TBL_LENGTH]; i++) ata[INSTRPAR + i] = ai.noteTable[i];

	//envelope is behind the table
	BOOL stereo = (g_tracks4_8 > 4);
	int len = par[PAR_ENV_LENGTH];
	for (i = 0, j = tablelast + 1; i <= len; i++, j += 3)
	{
		int* env = (int*)&ai.envelope[i];
		ata[j] = (stereo) ?
			(env[ENV_VOLUMER] << 4) | (env[ENV_VOLUMEL])	//stereo
			:
			(env[ENV_VOLUMEL] << 4) | (env[ENV_VOLUMEL]); //mono, VOLUME R = VOLUME L

		ata[j + 1] = (env[ENV_FILTER] << 7)
			| (env[ENV_COMMAND] << 4)	//0-7
			| (env[ENV_DISTORTION])	//0,2,4,6,8,A,C,E
			| (env[ENV_PORTAMENTO]);
		ata[j + 2] = (env[ENV_X] << 4)
			| (env[ENV_Y]);
	}
	return tablelast + 1 + (len + 1) * 3;	//returns the data length of the instrument
}

BYTE CInstruments::InstrToAtaRMF(int instr, unsigned char* ata, int max)
{
	TInstrument& ai = m_instr[instr];
	int i, j;
	int* par = ai.parameters;

	/*							RMF
	  0 IDXTABLEEND
	  1 IDXTABLEGO
	  2 IDXENVEND				+1
	  3 IDXENVGO
	  4 TABTYPEMODESPEED
	  5 AUDCTL
	  6 VSLIDE
	  7 VMIN
	  8 EFFDELAY
	  9 EFVIBRATO
	 10 FSHIFT
	 11 0 (unused)				omitted
	*/

	const int INSTRPAR = 11;			//RMF (default is 12)

	int tablelast = par[PAR_TBL_LENGTH] + INSTRPAR;	 //+12	//12th byte starts the table
	ata[0] = tablelast;
	ata[1] = par[PAR_TBL_GOTO] + INSTRPAR;				//12th byte starts the table
	ata[2] = par[PAR_ENV_LENGTH] * 3 + tablelast + 1 + 1;	//behind the table is the envelope // RMF +1
	ata[3] = par[PAR_ENV_GOTO] * 3 + tablelast + 1;
	//
	ata[4] = (par[PAR_TBL_TYPE] << 7)
		| (par[PAR_TBL_MODE] << 6)
		| (par[PAR_TBL_SPEED]);
	//
	ata[5] = par[PAR_AUDCTL_15KHZ]
		| (par[PAR_AUDCTL_HPF_CH2] << 1)
		| (par[PAR_AUDCTL_HPF_CH1] << 2)
		| (par[PAR_AUDCTL_JOIN_3_4] << 3)
		| (par[PAR_AUDCTL_JOIN_1_2] << 4)
		| (par[PAR_AUDCTL_179_CH3] << 5)
		| (par[PAR_AUDCTL_179_CH1] << 6)
		| (par[PAR_AUDCTL_POLY9] << 7);
	ata[6] = par[PAR_VOL_FADEOUT];
	ata[7] = par[PAR_VOL_MIN] << 4;
	ata[8] = par[PAR_DELAY];
	ata[9] = par[PAR_VIBRATO] & 0x03;
	ata[10] = par[PAR_FREQ_SHIFT];
	ata[11] = 0; //unused

	//write for the entire length of the table
	for (i = 0; i <= par[PAR_TBL_LENGTH]; i++) ata[INSTRPAR + i] = ai.noteTable[i];

	//envelope is behind the table
	BOOL stereo = (g_tracks4_8 > 4);
	int len = par[PAR_ENV_LENGTH];
	for (i = 0, j = tablelast + 1; i <= len; i++, j += 3)
	{
		int* env = (int*)&ai.envelope[i];
		ata[j] = (stereo) ?
			(env[ENV_VOLUMER] << 4) | (env[ENV_VOLUMEL]) //stereo
			:
			(env[ENV_VOLUMEL] << 4) | (env[ENV_VOLUMEL]); //mono, VOLUME R = VOLUME L

		ata[j + 1] = (env[ENV_FILTER] << 7)
			| (env[ENV_COMMAND] << 4)	//0-7
			| (env[ENV_DISTORTION])	//0,2,4,..14
			| (env[ENV_PORTAMENTO]);
		ata[j + 2] = (env[ENV_X] << 4)
			| (env[ENV_Y]);
	}
	return tablelast + 1 + (len + 1) * 3;	//returns the data length of the instrument
}

/// <summary>
/// The instrument was modified in some way.
/// Push the new instrument data to Atari
/// and update the display hint flag.
/// </summary>
/// <param name="instr">Instrument #</param>
/// <returns></returns>
void CInstruments::WasModified(int instr)
{
	unsigned char* ata = g_atarimem + instr * 256 + 0x4000;
	g_rmtroutine = 0;			//turn off RMT routines
	InstrToAta(instr, ata, MAXATAINSTRLEN);
	g_rmtroutine = 1;			//RMT routines are turned on

	RecalculateFlag(instr);
}