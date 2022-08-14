// POKEY Frequencies Calculator
// by VinsCool
// Based on the code originally used for the Pitch Calculations in Raster Music Tracker 1.31+
// Backported to RMT with additional improvements

#include "tuning.h"
using namespace std;

//RMT's own notes chars will be used instead
const char* t_notes[] =
{
	"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"
};

//arrays of tuning tables to generate...
//
//PAGE_DISTORTION_2 => Address 0xB000
int tab_64khz_2[64] = { 0 };
int tab_179mhz_2[64] = { 0 };
int tab_16bit_2[128] = { 0 };
//
//PAGE_DISTORTION_4 => Address 0xB100
int tab_64khz_4_smooth[64] = { 0 };
int tab_179mhz_4_smooth[64] = { 0 };
int tab_16bit_4_smooth[128] = { 0 };
//
int tab_64khz_4_buzzy[64] = { 0 };
int tab_179mhz_4_buzzy[64] = { 0 };
int tab_16bit_4_buzzy[128] = { 0 };
//
//PAGE_DISTORTION_A => Address 0xB200
int tab_64khz_a_pure[64] = { 0 };
int tab_179mhz_a_pure[64] = { 0 };
int tab_16bit_a_pure[128] = { 0 };
//
//PAGE_DISTORTION_C => Address 0xB300
int tab_64khz_c_buzzy[64] = { 0 };
int tab_179mhz_c_buzzy[64] = { 0 };
int tab_16bit_c_buzzy[128] = { 0 };
//
//PAGE_DISTORTION_E => Address 0xB400
int tab_64khz_c_gritty[64] = { 0 };
int tab_179mhz_c_gritty[64] = { 0 };
int tab_16bit_c_gritty[128] = { 0 };
//
//PAGE_EXTRA_0 => Address 0xB500, 0xB580 for 15khz Pure and 0xB5C0 for 15khz Buzzy
//Croissant Sawtooth ch1	//no generator yet
//Croissant Sawtooth ch3	//no generator yet
int tab_15khz_a_pure[64] = { 0 };
int tab_15khz_c_buzzy[64] = { 0 };
//
//PAGE_EXTRA_1 => Address 0xB600
//Clarinet Lo	//no generator yet
//Clarinet Hi	//no generator yet
//unused
//unused
//
//PAGE_EXTRA_2 => Address 0xB700
int tab_64khz_c_unstable[64] = { 0 };
int tab_179mhz_c_unstable[64] = { 0 };
int tab_16bit_c_unstable[128] = { 0 };
//

int FREQ_17 = (g_ntsc) ? FREQ_17_NTSC : FREQ_17_PAL; 

//this should be more than enough for all possible values...
double ref_pitch[2200] = { 0 };

//AUDCTL bits
bool CLOCK_15 = 0;				//0x01
bool HPF_CH24 = 0;				//0x02
bool HPF_CH13 = 0;				//0x04
bool JOIN_34 = 0;				//0x08
bool JOIN_12 = 0;				//0x10
bool CH3_179 = 0;				//0x20
bool CH1_179 = 0;				//0x40
bool POLY9 = 0;					//0x80

//combined AUDCTL bits for special cases
bool JOIN_16BIT = 0;			//valid 16-bit mode
bool CLOCK_179 = 0;				//valid 1.79mhz mode
bool SAWTOOTH = 0;				//valid Sawtooth mode
bool SAWTOOTH_INVERTED = 0;		//valid Sawtooth mode (inverted)

//SKCTL bits
bool TWO_TONE = 0;				//0x8B, Two-Tone Filter

//combined AUDC and AUDF bits for special cases
bool IS_SMOOTH_DIST_4 = 0;
bool IS_BUZZY_DIST_4 = 0;
bool IS_UNSTABLE_DIST_4_1 = 0;
bool IS_UNSTABLE_DIST_4_2 = 0;
bool IS_BUZZY_DIST_C = 0;		//Distortion C, MOD3 AUDF and not MOD5 AUDF
bool IS_GRITTY_DIST_C = 0;		//Distortion C, neither MOD3 nor MOD5 AUDF
bool IS_UNSTABLE_DIST_C = 0;	//Distortion C, MOD5 AUDF and not MOD3 AUDF
bool IS_METALLIC_POLY9 = 0;		//AUDCTL 0x80, Distortion 0 and 8, MOD7 AUDF

//Thomas Young 1799's Well Temperament no.1
const double YOUNG1[13] =
{
	1,
	1.055709,
	1.119770,
	1.187690,
	1.253887,
	1.334739,
	1.407637,
	1.496513,
	1.583581,
	1.675715,
	1.781546,
	1.878851,
	2
};

//Thomas Young 1799's Well Temperament no.2 
const double YOUNG2[13] =
{
	1,
	1.055880,
	1.119929,
	1.187865,
	1.254242,
	1.334839,
	1.407840,
	1.496616,
	1.583819,
	1.676105,
	1.781797,
	1.879240,
	2
};

//Thomas Young 1807's Well Temperament
const double YOUNG3[13] =
{
	1,
	1.053497,
	1.119929,
	1.185185,
	1.254242,
	1.333333,
	1.404663,
	1.496616,
	1.580246,
	1.676105,
	1.777777,
	1.877119,
	2
};

//Andreas Werckmeister's temperament III (the most famous one, 1681)
const double WERCK3[13] =
{
	1,
	1.053497,
	1.117403,
	1.185185,
	1.252827,
	1.333333,
	1.404663,
	1.494927,
	1.580246,
	1.670436,
	1.777777,
	1.879240,
	2
};

//Temp�rament �gal � Quintes Justes 
const double QUINTE[13] =
{
	1,
	1.059634,
	1.122824,
	1.189782,
	1.260734,
	1.335916,
	1.415582,
	1.5,
	1.589451,
	1.684236,
	1.784674,
	1.891101,
	2.003875
};

//d'Alembert and Rousseau temp�rament ordinaire (1752/1767)
const double TEMPORD[13] =
{
	1,
	1.051120,
	1.118034,
	1.181176,
	1.250000,
	1.331828,
	1.403077,
	1.495348,
	1.574901,
	1.671850,
	1.773766,
	1.872883,
	2
};

//Aron - Neidhardt equal beating well temperament
const double ARONIED[13] =
{
	1,
	1.053497,
	1.118144,
	1.185185,
	1.250031,
	1.333333,
	1.404663,
	1.496112,
	1.580246,
	1.672002,
	1.777777,
	1.872885,
	2
};

//Atom Schisma Scale
const double ATOMSCH[13] =
{
	1,
	1.059459,
	1.122463,
	1.189204,
	1.259924,
	1.334838,
	1.414207,
	1.498308,
	1.587396,
	1.681796,
	1.781794,
	1.887755,
	2
};

//12 - tET approximation with minimal order 17 beats
const double APPRX12[13] =
{
	1,
	1.058823,
	1.125000,
	1.187500,
	1.250000,
	1.333333,
	1.416666,
	1.500000,
	1.588235,
	1.666666,
	1.777777,
	1.888888,
	2
};

//Paul Bailey's modern well temperament (2002)
const double BAILEY1[13] =
{
	1,
	1.054892,
	1.119671,
	1.186774,
	1.254242,
	1.335183,
	1.406531,
	1.498302,
	1.582329,
	1.676250,
	1.780202,
	1.881407,
	2
};

//John Barnes' temperament (1977) made after analysis of Wohltemperierte Klavier, 1/6 P
const double BARNES1[13] =
{
	1,
	1.055880,
	1.119929,
	1.187865,
	1.254242,
	1.336348,
	1.407840,
	1.496616,
	1.583819,
	1.676105,
	1.781797,
	1.881364,
	2
};

//Bethisy temperament ordinaire, see Pierre - Yves Asselin : Musique et temperament
const double BETHISY[13] =
{
	1,
	1.051418,
	1.118034,
	1.181509,
	1.250000,
	1.331953,
	1.403475,
	1.495348,
	1.575346,
	1.671850,
	1.774101,
	1.872884,
	2,
};

//Big Gulp
const double BIGGULP[13] =
{
	1,
	1.031250,
	1.125000,
	1.166666,
	1.250000,
	1.312500,
	1.375000,
	1.500000,
	1.546875,
	1.687500,
	1.750000,
	1.875000,
	2
};

//12 - tone scale by Bohlen generated from the 4:7 : 10 triad, Acustica 39 / 2, 1978
const double BOHLEN12[13] =
{
	1,
	1.100000,
	1.200000,
	1.304347,
	1.428571,
	1.571428,
	1.750000,
	1.909090,
	2.100000,
	2.300000,
	2.500000,
	2.750000,
	3
};

//This scale may also be called the "Wedding Cake"
const double WEDDING[13] =
{
	1,
	1.125000,
	1.171875,
	1.250000,
	1.333333,
	1.406250,
	1.500000,
	1.562500,
	1.666666,
	1.687500,
	1.777777,
	1.875000,
	2
};

//Upside - Down Wedding Cake(divorce cake)
const double DIVORCE[13] =
{
	1,
	1.066666,
	1.125000,
	1.200000,
	1.280000,
	1.333333,
	1.500000,
	1.600000,
	1.687500,
	1.777777,
	1.800000,
	1.920000,
	2
};

//12 - tone Pythagorean scale
const double PYTHAG1[13] =
{
	1,
	1.067871,
	1.125000,
	1.185185,
	1.265625,
	1.333333,
	1.423828,
	1.500000,
	1.601806,
	1.687500,
	1.777777,
	1.898437,
	2
};

//Robert Schneider, scale of log(4) ..log(16), 1 / 1 = 264Hz
const double LOGSCALE[13] =
{
	1,
	1.160964,
	1.292481,
	1.403677,
	1.500000,
	1.584962,
	1.660964,
	1.729715,
	1.792481,
	1.850219,
	1.903677,
	1.953445,
	2
};

//Zarlino temperament extraordinaire, 1024 - tET mapping
const double ZARLINO1[13] =
{
	1,
	1.041450,
	1.116652,
	1.180385,
	1.247756,
	1.337855,
	1.393309,
	1.494930,
	1.567469,
	1.669316,
	1.777781,
	1.865308,
	2
};

//Fokker's 7-limit 12-tone just scale
const double FOKKER7[13] =
{
	1,
	1.071428,
	1.125000,
	1.166666,
	1.250000,
	1.333333,
	1.406250,
	1.500000,
	1.607142,
	1.666666,
	1.750000,
	1.875000,
	2
};

//Bach temperament, a'=400 Hz
const double BACH400[13] =
{
	1,
	1.052192,
	1.118997,
	1.183716,
	1.250521,
	1.334029,
	1.402922,
	1.498956,
	1.578288,
	1.670146,
	1.776618,
	1.874739,
	2
};

//Vallotti& Young scale(Vallotti version) also known as Tartini - Vallotti(1754)
const double VALYOUNG[13] =
{
	1,
	1.055880,
	1.119929,
	1.187865,
	1.254242,
	1.336348,
	1.407840,
	1.496616,
	1.583819,
	1.676105,
	1.781797,
	1.877119,
	2
};

//Vallotti - Young and Werckmeister III, 10 cents 5 - limit lesfip scale
const double VALYOWER[13] =
{
	1,
	1.051637,
	1.117298,
	1.187060,
	1.248356,
	1.336846,
	1.399836,
	1.495457,
	1.580099,
	1.669531,
	1.783574,
	1.867613,
	2
};


///!!!\\\ Testing some non-12 octaves scales here

//Optimally consonant major pentatonic, John deLaubenfels(2001)
const double PENTAOPT[6] =
{
	1,
	1.118042,
	1.250019,
	1.496879,
	1.670166,
	2
};

//Ancient Greek Aeolic, also tritriadic scale of the 54:64 : 81 triad
const double AEOLIC[8] =
{
	1,
	1.125000,
	1.185185,
	1.333333,
	1.500000,
	1.580246,
	1.777777,
	2
};

//African Bapare xylophone(idiophone; loose log)
const double XYLO1[11] =
{
	1,
	1.076737,
	1.200942,
	1.336382,
	1.497441,
	1.670175,
	1.932988,
	2.174725,
	2.285484,
	2.525670,
	2.738400
};

//African Yaswa xylophones(idiophone; calbash resonators with membrane)
const double XYLO2[11] =
{
	1,
	1.128312,
	1.271619,
	1.486239,
	1.707240,
	1.936341,
	2.015074,
	2.215296,
	2.419988,
	2.871225,
	3.220980
};

//19-EDO generated using Scale Workshop
const double NINTENDO[20] =
{
	1,
	1.037155,
	1.075690,
	1.115657,
	1.157110,
	1.200102,
	1.244692,
	1.290939,
	1.338904,
	1.388651,
	1.440246,
	1.493759,
	1.549259,
	1.606822,
	1.666524,
	1.728443,
	1.792664,
	1.859270,
	1.928352,
	2
};

//Parts of this code was rewritten for POKEY Frequencies Calculator, then backported to RMT 1.31+
void real_freq()
{
	double PITCH = 0;
	double centnum = 0;
	int notesnum = 12;	//unless specified otherwise
	int offset = g_basenote; //offset base tuning note. eg: A- = 3, C- = 0, D# = 9 etc
	int temperament = g_temperament;	//custom is always 0

	if (temperament)	//tuning by ratio, eg: Just Intonation, Pythagorean, etc
	{
		//some variables for the calcualtions...
		int interval = 0;	//keep track of the interval regardless of the basenote offset
		double multi = 1.0;	//octave multiplyer for the ratio
		double octave = 2;
		double ratio[31] = { 1 };

		for (int i = 0; i < 31; i++) 
		{ 
			//TODO: fix a bug with Distortion C, most likely caused by garbage data in memory when generating tables-- note from June 12th 2022: I don't remember if I actually have fixed this already, to confirm...
			//TODO? fix non-12 notes octaves scales
			//BUG: notes below C-0 still appear as octave 0 for some reason, meaning Octave 0 is displayed twice before becoming -1
			//FIX: "div" and "multi" are pointless, and break things actually, but this was not so obvious at first

			switch (temperament)
			{
			case 1:		//Thomas Young 1799's Well Temperament no.1
				if (i > 12) break;
				ratio[i] = YOUNG1[i]; 
				octave = YOUNG1[12];
				break;

			case 2:		//Thomas Young 1799's Well Temperament no.2
				if (i > 12) break;
				ratio[i] = YOUNG2[i];
				octave = YOUNG2[12];
				break;

			case 3:		//Thomas Young 1807's Well Temperament
				if (i > 12) break;
				ratio[i] = YOUNG3[i];
				octave = YOUNG3[12];
				break;

			case 4:		//Andreas Werckmeister's temperament III (the most famous one, 1681)
				if (i > 12) break;
				ratio[i] = WERCK3[i];
				octave = WERCK3[12];
				break;

			case 5:		//Temp�rament �gal � Quintes Justes 
				if (i > 12) break;
				ratio[i] = QUINTE[i];
				octave = QUINTE[12];
				break;

			case 6:		//d'Alembert and Rousseau temp�rament ordinaire (1752/1767)
				if (i > 12) break;
				ratio[i] = TEMPORD[i];
				octave = TEMPORD[12];
				break;

			case 7:		//Aron - Neidhardt equal beating well temperament
				if (i > 12) break;
				ratio[i] = ARONIED[i];
				octave = ARONIED[12];
				break;

			case 8:		//Atom Schisma Scale
				if (i > 12) break;
				ratio[i] = ATOMSCH[i];
				octave = ATOMSCH[12];
				break;

			case 9:		//12 - tET approximation with minimal order 17 beats
				if (i > 12) break;
				ratio[i] = APPRX12[i];
				octave = APPRX12[12];
				break;

			case 10:	//Paul Bailey's modern well temperament (2002)
				if (i > 12) break;
				ratio[i] = BAILEY1[i];
				octave = BAILEY1[12];
				break;

			case 11:	//John Barnes' temperament (1977) made after analysis of Wohltemperierte Klavier, 1/6 P
				if (i > 12) break;
				ratio[i] = BARNES1[i];
				octave = BARNES1[12];
				break;

			case 12:	//Bethisy temperament ordinaire, see Pierre - Yves Asselin : Musique et temperament
				if (i > 12) break;
				ratio[i] = BETHISY[i];
				octave = BETHISY[12];
				break;

			case 13:	//Big Gulp
				if (i > 12) break;
				ratio[i] = BIGGULP[i];
				octave = BIGGULP[12];
				break;

			case 14:	//12 - tone scale by Bohlen generated from the 4:7 : 10 triad, Acustica 39 / 2, 1978
				if (i > 12) break;
				ratio[i] = BOHLEN12[i];
				octave = BOHLEN12[12];
				break;

			case 15:	//This scale may also be called the "Wedding Cake"
				if (i > 12) break;
				ratio[i] = WEDDING[i];
				octave = WEDDING[12];
				break;

			case 16:	//Upside - Down Wedding Cake(divorce cake)
				if (i > 12) break;
				ratio[i] = DIVORCE[i];
				octave = DIVORCE[12];
				break;

			case 17:	//12 - tone Pythagorean scale
				if (i > 12) break;
				ratio[i] = PYTHAG1[i];
				octave = PYTHAG1[12];
				break;

			case 18:	//Robert Schneider, scale of log(4) ..log(16), 1 / 1 = 264Hz
				if (i > 12) break;
				ratio[i] = LOGSCALE[i];
				octave = LOGSCALE[12];
				break;

			case 19:	//Zarlino temperament extraordinaire, 1024 - tET mapping
				if (i > 12) break;
				ratio[i] = ZARLINO1[i];
				octave = ZARLINO1[12];
				break;

			case 20:	//Fokker's 7-limit 12-tone just scale
				if (i > 12) break;
				ratio[i] = FOKKER7[i];
				octave = FOKKER7[12];
				break;

			case 21:	//Bach temperament, a'=400 Hz
				if (i > 12) break;
				ratio[i] = BACH400[i];
				octave = BACH400[12];
				break;

			case 22:	//Vallotti& Young scale(Vallotti version) also known as Tartini - Vallotti(1754)
				if (i > 12) break;
				ratio[i] = VALYOUNG[i];
				octave = VALYOUNG[12];
				break;

			case 23:	//Vallotti - Young and Werckmeister III, 10 cents 5 - limit lesfip scale
				if (i > 12) break;
				ratio[i] = VALYOWER[i];
				octave = VALYOWER[12];
				break;

//

			case 24:	//Optimally consonant major pentatonic, John deLaubenfels(2001)
				if (i > 5) break;
				ratio[i] = PENTAOPT[i];
				octave = PENTAOPT[5];
				notesnum = 5;
				//div = 16;
				break;

			case 25:	//Ancient Greek Aeolic, also tritriadic scale of the 54:64 : 81 triad
				if (i > 7) break;
				ratio[i] = AEOLIC[i];
				octave = AEOLIC[7];
				notesnum = 7;
				//div = 4;
				break;

			case 26:	//African Bapare xylophone(idiophone; loose log)
				if (i > 10) break;
				ratio[i] = XYLO1[i];
				octave = XYLO1[10];
				notesnum = 10;
				//div = 8;
				break;

			case 27:	//African Yaswa xylophones(idiophone; calbash resonators with membrane)
				if (i > 10) break;
				ratio[i] = XYLO2[i];
				octave = XYLO2[10];
				notesnum = 10;
				//div = 8;
				break;

			case 28:	//19-EDO generated using Scale Workshop
				if (i > 19) break;
				ratio[i] = NINTENDO[i];
				octave = NINTENDO[19];
				notesnum = 19;
				//div = 0.25;
				break;
//

			default:	//custom, ratio used for each note => NOTE_L / NOTE_R, must be treated as doubles!!!
				if (i > 12) break;
				ratio[0] = (double)g_UNISON_L / (double)g_UNISON_R;
				ratio[1] = (double)g_MIN_2ND_L / (double)g_MIN_2ND_R;
				ratio[2] = (double)g_MAJ_2ND_L / (double)g_MAJ_2ND_R;
				ratio[3] = (double)g_MIN_3RD_L / (double)g_MIN_3RD_R;
				ratio[4] = (double)g_MAJ_3RD_L / (double)g_MAJ_3RD_R;
				ratio[5] = (double)g_PERF_4TH_L / (double)g_PERF_4TH_R;
				ratio[6] = (double)g_TRITONE_L / (double)g_TRITONE_R;
				ratio[7] = (double)g_PERF_5TH_L / (double)g_PERF_5TH_R;
				ratio[8] = (double)g_MIN_6TH_L / (double)g_MIN_6TH_R;
				ratio[9] = (double)g_MAJ_6TH_L / (double)g_MAJ_6TH_R;
				ratio[10] = (double)g_MIN_7TH_L / (double)g_MIN_7TH_R;
				ratio[11] = (double)g_MAJ_7TH_L / (double)g_MAJ_7TH_R;
				octave = (double)g_OCTAVE_L / (double)g_OCTAVE_R;
				break;
			}

		} 
		for (int i = 0; i < 180; i++)
		{
			double tuning = g_basetuning / 64;
			PITCH = (multi * ratio[interval]) * tuning;
			ref_pitch[(i - offset) * 12] = PITCH;
			interval++;
			if (interval == notesnum)	//notes per octave, or whatever ratio at the so called octave interval
			{
				interval = 0;	//reset the interval count
				multi = multi * octave;	//multiply by itself
			}
		}
	}
	else	//12TET
	{
		double ratio = pow(2.0, 1.0 / 12.0);
		for (int i = 0; i < 180; i++)
		{
			double tuning = g_basetuning / 64;
			double PITCH = tuning * pow(ratio, i + offset);
			ref_pitch[i * 12] = PITCH;
		}
	}
}

//Parts of this code was rewritten for POKEY Frequencies Calculator, then backported to RMT 1.31+
double generate_freq(int i_audc, int i_audf, int i_audctl, int i_ch_index)
{
	//register variables
	int i = i_ch_index;
	int audctl = i_audctl;
	int skctl = 0;
	int audf = i_audf;
	int audf16 = i_audf;		//a 16bit number is fed into it directly instead
	int audc = i_audc;
	int dist = audc & 0xf0;
	int modoffset = 0;

	//variables for pitch calculation
	double PITCH = 0;
	double divisor = 0;
	int coarse_divisor = 0;

	IS_UNSTABLE_DIST_4_2 = 0;

	IS_BUZZY_DIST_C = 0;
	IS_GRITTY_DIST_C = 0;
	IS_UNSTABLE_DIST_C = 0;
	IS_METALLIC_POLY9 = 0;

	CLOCK_15 = audctl & 0x01;
	HPF_CH24 = audctl & 0x02;
	HPF_CH13 = audctl & 0x04;
	JOIN_34 = audctl & 0x08;
	JOIN_12 = audctl & 0x10;
	CH3_179 = audctl & 0x20;
	CH1_179 = audctl & 0x40;
	POLY9 = audctl & 0x80;

	TWO_TONE = (skctl == 0x8B) ? 1 : 0;

	//combined modes for some special output...
	JOIN_16BIT = ((JOIN_12 && CH1_179 && (i == 1 || i == 5)) || (JOIN_34 && CH3_179 && (i == 3 || i == 7))) ? 1 : 0;
	CLOCK_179 = ((CH1_179 && (i == 0 || i == 4)) || (CH3_179 && (i == 2 || i == 6))) ? 1 : 0;
	if (JOIN_16BIT || CLOCK_179) CLOCK_15 = 0;	//override, these 2 take priority over 15khz mode if they are enabled at the same time

	/*
	//TODO: Sawtooth generation needs to be optimal in order to compromise the high pitched hiss versus the tuning accuracy
	SAWTOOTH = (CH1_179 && CH3_179 && HPF_CH13 && (dist == 0xA0 || dist == 0xE0) && (i == 0 || i == 4)) ? 1 : 0;
	SAWTOOTH_INVERTED = 0;
	if (i % 4 == 0)	//only in valid sawtooth channels
	audf3 = g_atarimem[idx[i + 2]];
	*/

	//TODO: apply Two-Tone timer offset into calculations when channel 1+2 are linked in 1.79mhz mode
	//This would help generating tables using patterns discovered by synthpopalooza
	modoffset = 1;
	coarse_divisor = 1;
	divisor = 1;
	int v_modulo = 0;
	bool IS_VALID = 0;

	if (JOIN_16BIT) modoffset = 7;
	else if (CLOCK_179) modoffset = 4;
	else coarse_divisor = (CLOCK_15) ? 114 : 28;

	switch (dist)
	{
	case 0x60:	//Duplicate of Distortion 2
	case 0x20:
		divisor = 31;
		v_modulo = 31;
		IS_VALID = ((audf + modoffset) % v_modulo == 0) ? 0 : 1;
		break;

	case 0x40:
		divisor = 232.5;		//Buzzy
		v_modulo = (CLOCK_15) ? 5 : 15;
		IS_UNSTABLE_DIST_4_1 = ((audf + modoffset) % 5 == 0) ? 1 : 0;
		IS_SMOOTH_DIST_4 = ((audf + modoffset) % 3 == 0 || CLOCK_15) ? 1 : 0;
		IS_UNSTABLE_DIST_4_2 = ((audf + modoffset) % 31 == 0) ? 1 : 0;
		if (IS_UNSTABLE_DIST_4_1) divisor = 46.5;	//Unstable #1
		if (IS_SMOOTH_DIST_4) divisor = 77.5;	//Smooth
		if (IS_UNSTABLE_DIST_4_2) divisor = (IS_SMOOTH_DIST_4 || IS_UNSTABLE_DIST_4_1) ? 2.5 : 7.5;	//Unstable #2 and #3		
		IS_VALID = ((audf + modoffset) % v_modulo == 0) ? 0 : 1;
		break;

	case 0x00:
	case 0x80:
		if (POLY9)
		{
			divisor = 255.5;	//Metallic Buzzy
			v_modulo = 73;
			if (CLOCK_179 || JOIN_16BIT)
				IS_METALLIC_POLY9 = ((audf + modoffset) % 7 == 0) ? 1 : 0;
			else
				IS_METALLIC_POLY9 = 1;

			if (IS_METALLIC_POLY9) divisor = 36.5;
			IS_VALID = ((audf + modoffset) % v_modulo == 0) ? 0 : 1;
			if (dist == 0x00 && ((audf + modoffset) % 31 == 0)) IS_VALID = 0;
		}
		else IS_VALID = 1;	//output is the same as Distortion A
		break;

	case 0xE0:	//Duplicate of Distortion A
	case 0xA0:
		IS_VALID = 1;
		break;

	case 0xC0:
		divisor = 7.5;		//Gritty
		v_modulo = (CLOCK_15) ? 5 : 15;
		IS_UNSTABLE_DIST_C = ((audf + modoffset) % 5 == 0) ? 1 : 0;
		IS_BUZZY_DIST_C = ((audf + modoffset) % 3 == 0 || CLOCK_15) ? 1 : 0;
		if (IS_UNSTABLE_DIST_C) divisor = 1.5;	//Unstable
		if (IS_BUZZY_DIST_C) divisor = 2.5;	//Buzzy
		IS_VALID = ((audf + modoffset) % v_modulo == 0) ? 0 : 1;
		break;
	}
	if (IS_VALID)
		PITCH = get_pitch(audf, coarse_divisor, divisor, modoffset);
	return PITCH;
}

//this code was originally added in POKEY Frequencies Calculator, and adapted for RMT 1.31+
void generate_table(int note, double freq, int distortion, bool IS_15KHZ, bool IS_179MHZ, bool IS_16BIT)
{
	int audf = 0;
	int modoffset = 1;
	int coarse_divisor = 1;
	int v_modulo = 0;
	double divisor = 1;
	double PITCH = 0;

	//since globals are specifically wanted, the parameters are used to define these flags here
	CLOCK_15 = IS_15KHZ;
	CLOCK_179 = IS_179MHZ;
	JOIN_16BIT = IS_16BIT;

	//TODO: apply Two-Tone timer offset into calculations when channel 1+2 are linked in 1.79mhz mode
	//This would help generating tables using patterns discovered by synthpopalooza
	if (JOIN_16BIT) modoffset = 7;
	else if (CLOCK_179) modoffset = 4;
	else coarse_divisor = (CLOCK_15) ? 114 : 28;

	switch (distortion)
	{
	case 0x20:
		divisor = 31;
		v_modulo = 31;
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if ((audf + modoffset) % v_modulo == 0)	//invalid values
			audf = delta_audf(audf, freq, coarse_divisor, divisor, modoffset, distortion);
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT) tab_16bit_2[note * 2] = audf;
		else if (CLOCK_179) tab_179mhz_2[note] = audf;
		else tab_64khz_2[note] = audf;
		break;

	case 0x40:
		divisor = (IS_SMOOTH_DIST_4 || CLOCK_15) ? 77.5 : 232.5;		//Smooth
		v_modulo = (CLOCK_15) ? 5 : 15;
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if ((CLOCK_15 && (audf + modoffset) % v_modulo == 0) ||
			(IS_SMOOTH_DIST_4 && ((audf + modoffset) % 3 != 0 || (audf + modoffset) % 5 == 0 || (audf + modoffset) % 31 == 0)) ||
			(IS_BUZZY_DIST_4 && ((audf + modoffset) % 3 == 0 || (audf + modoffset) % 5 == 0 || (audf + modoffset) % 31 == 0))) 
		{
			audf = delta_audf(audf, freq, coarse_divisor, divisor, modoffset, distortion); 
		}
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT)
		{
			if (IS_SMOOTH_DIST_4) tab_16bit_4_smooth[note * 2] = audf;
			else if (IS_BUZZY_DIST_4) tab_16bit_4_buzzy[note * 2] = audf;
			else break;	//invalid parameter
		}
		else if (CLOCK_179)
		{
			if (IS_SMOOTH_DIST_4) tab_179mhz_4_smooth[note] = audf;
			else if (IS_BUZZY_DIST_4) tab_179mhz_4_buzzy[note] = audf;
			else break;	//invalid parameter
		}
		else	//64khz mode
		{
			if (IS_SMOOTH_DIST_4) tab_64khz_4_smooth[note] = audf;
			else if (IS_BUZZY_DIST_4) tab_64khz_4_buzzy[note] = audf;
			else break;	//invalid parameter
		}
		break;

	case 0xA0:
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT) tab_16bit_a_pure[note * 2] = audf;
		else if (CLOCK_179) tab_179mhz_a_pure[note] = audf;
		else if (CLOCK_15) tab_15khz_a_pure[note] = audf;
		else tab_64khz_a_pure[note] = audf;
		break;

	case 0xC0:
		divisor = (IS_BUZZY_DIST_C || CLOCK_15) ? 2.5 : 7.5;
		v_modulo = (CLOCK_15) ? 5 : 15;
		if (IS_UNSTABLE_DIST_C) divisor = 1.5;
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if ((CLOCK_15 && (audf + modoffset) % v_modulo == 0) ||
			(IS_BUZZY_DIST_C && ((audf + modoffset) % 3 != 0 || (audf + modoffset) % 5 == 0)) ||
			(IS_GRITTY_DIST_C && ((audf + modoffset) % 3 == 0 || (audf + modoffset) % 5 == 0)) ||
			(IS_UNSTABLE_DIST_C && ((audf + modoffset) % 3 == 0 || (audf + modoffset) % 5 != 0)))
		{
			audf = delta_audf(audf, freq, coarse_divisor, divisor, modoffset, distortion); //aaaaaa
		}
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT)
		{
			if (IS_BUZZY_DIST_C) tab_16bit_c_buzzy[note * 2] = audf;
			else if (IS_GRITTY_DIST_C) tab_16bit_c_gritty[note * 2] = audf;
			else if (IS_UNSTABLE_DIST_C) tab_16bit_c_unstable[note * 2] = audf;
			else break;	//invalid parameter
		}
		else if (CLOCK_179)
		{
			if (IS_BUZZY_DIST_C) tab_179mhz_c_buzzy[note] = audf;
			else if (IS_GRITTY_DIST_C) tab_179mhz_c_gritty[note] = audf;
			else if (IS_UNSTABLE_DIST_C) tab_179mhz_c_unstable[note] = audf;
			else break;	//invalid parameter
		}
		else if (CLOCK_15) tab_15khz_c_buzzy[note] = audf;
		else	//64khz mode
		{
			if (IS_BUZZY_DIST_C) tab_64khz_c_buzzy[note] = audf;
			else if (IS_GRITTY_DIST_C) tab_64khz_c_gritty[note] = audf;
			else if (IS_UNSTABLE_DIST_C) tab_64khz_c_unstable[note] = audf;
			else break;	//invalid parameter
		}
		break;
	}
}

//code originally written for POKEY Frequencies Calculator
double get_pitch(int audf, int coarse_divisor, double divisor, int modoffset)
{
	FREQ_17 = (g_ntsc) ? FREQ_17_NTSC : FREQ_17_PAL;
	double PITCH = ((FREQ_17 / (coarse_divisor * divisor)) / (audf + modoffset)) / 2;
	return PITCH;
}

//code originally written for POKEY Frequencies Calculator
int get_audf(double freq, int coarse_divisor, double divisor, int modoffset)
{
	FREQ_17 = (g_ntsc) ? FREQ_17_NTSC : FREQ_17_PAL;
	int audf = (int)round(((FREQ_17 / (coarse_divisor * divisor)) / (2 * freq)) - modoffset);
	return audf;
}

//code originally written for POKEY Frequencies Calculator
int delta_audf(int audf, double freq, int coarse_divisor, double divisor, int modoffset, int distortion)
{
	int tmp_audf_up = audf;		//begin from the currently invalid audf
	int tmp_audf_down = audf;
	double tmp_freq_up = 0;
	double tmp_freq_down = 0;
	double PITCH = 0;

	if (distortion != 0x40 && distortion != 0xC0) { tmp_audf_up++; tmp_audf_down--; }	//anything not distortion 4 or C, simpliest delta method

	else if (distortion == 0x40)
	{
		if (IS_SMOOTH_DIST_4)	//verify MOD3 integrity
		{
			for (int o = 0; o < 6; o++)
			{
				if ((tmp_audf_up + modoffset) % 3 != 0 || (tmp_audf_up + modoffset) % 5 == 0 || (tmp_audf_up + modoffset) % 31 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 != 0 || (tmp_audf_down + modoffset) % 5 == 0 || (tmp_audf_down + modoffset) % 31 == 0) tmp_audf_down--;
			}
		}
		else if (IS_BUZZY_DIST_4)
		{
			for (int o = 0; o < 6; o++) 
			{
				if ((tmp_audf_up + modoffset) % 3 == 0 || (tmp_audf_up + modoffset) % 5 == 0 || (tmp_audf_up + modoffset) % 31 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 == 0 || (tmp_audf_down + modoffset) % 5 == 0 || (tmp_audf_down + modoffset) % 31 == 0) tmp_audf_down--;
			}
		}
		else return 0;	//invalid parameter most likely 
	}

	else if (distortion == 0xC0)
	{
		if (CLOCK_15)
		{
			for (int o = 0; o < 3; o++)	//MOD5 must be avoided!
			{
				if ((tmp_audf_up + modoffset) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (IS_BUZZY_DIST_C)	//verify MOD3 integrity
		{
			for (int o = 0; o < 6; o++)
			{
				if ((tmp_audf_up + modoffset) % 3 != 0 || (tmp_audf_up + modoffset) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 != 0 || (tmp_audf_down + modoffset) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (IS_GRITTY_DIST_C)	//verify neither MOD3 or MOD5 is used
		{
			for (int o = 0; o < 6; o++)	//get the closest compromise up and down first
			{
				if ((tmp_audf_up + modoffset) % 3 == 0 || (tmp_audf_up + modoffset) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 == 0 || (tmp_audf_down + modoffset) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (IS_UNSTABLE_DIST_C)	//verify MOD5 integrity
		{
			for (int o = 0; o < 6; o++)	//get the closest compromise up and down first
			{
				if ((tmp_audf_up + modoffset) % 3 == 0 || (tmp_audf_up + modoffset) % 5 != 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 == 0 || (tmp_audf_down + modoffset) % 5 != 0) tmp_audf_down--;
			}
		}
		else return 0;	//invalid parameter most likely
	}

	PITCH = get_pitch(tmp_audf_up, coarse_divisor, divisor, modoffset);
	tmp_freq_up = freq - PITCH;	//first delta, up
	PITCH = get_pitch(tmp_audf_down, coarse_divisor, divisor, modoffset);
	tmp_freq_down = PITCH - freq;	//second delta, down
	PITCH = tmp_freq_down - tmp_freq_up;

	if (PITCH > 0) audf = tmp_audf_up; //positive, meaning delta up is closer than delta down
	else audf = tmp_audf_down; //negative, meaning delta down is closer than delta up
	return audf;
}

//code originally written for POKEY Frequencies Calculator, changes have been done to adapt it for RMT 1.31+
void macro_table_gen(int distortion, int note_offset, bool IS_15KHZ, bool IS_179MHZ, bool IS_16BIT)
{
	//generate the array
	for (int i = 0; i < 64; i++)
	{
		int note = i + note_offset;
		double freq = ref_pitch[note * 12];
		generate_table(i, freq, distortion, IS_15KHZ, IS_179MHZ, IS_16BIT);
	}
}

void init_tuning()
{
	//reset all the tables so no leftover will stay in memory...
	memset(tab_64khz_2, 0, 64);
	memset(tab_179mhz_2, 0, 64);
	memset(tab_16bit_2, 0, 128);
	memset(tab_64khz_4_smooth, 0, 64);
	memset(tab_179mhz_4_smooth, 0, 64);
	memset(tab_16bit_4_smooth, 0, 128);
	memset(tab_64khz_4_buzzy, 0, 64);
	memset(tab_179mhz_4_buzzy, 0, 64);
	memset(tab_16bit_4_buzzy, 0, 128);
	memset(tab_64khz_a_pure, 0, 64);
	memset(tab_179mhz_a_pure, 0, 64);
	memset(tab_16bit_a_pure, 0, 128);
	memset(tab_64khz_c_buzzy, 0, 64);
	memset(tab_179mhz_c_buzzy, 0, 64);
	memset(tab_16bit_c_buzzy, 0, 128);
	memset(tab_64khz_c_gritty, 0, 64);
	memset(tab_179mhz_c_gritty, 0, 64);
	memset(tab_16bit_c_gritty, 0, 128);
	memset(tab_15khz_a_pure, 0, 64);
	memset(tab_15khz_c_buzzy, 0, 64);

	//if base tuning is null, make sure to reset it, else the program could crash!
	if (!g_basetuning)
	{
		g_basetuning = (g_ntsc) ? 444.895778867913 : 440.83751645933;
		g_basenote = 3;	//3 = A-
		g_temperament = 0;	//no temperament 
		MessageBox(g_hwnd, "An invalid tuning configuration has been detected!\n\nTuning has been reset to default parameters.", "Tuning error", MB_ICONERROR); 
	}

	real_freq();	//generate the tuning reference in memory first
		
	//All the tables that could be calculated will be generated inside this entire block
	for (int d = 0x00; d < 0xE0; d += 0x20)
	{
		if (d == 0x00 || d == 0x60 || d == 0x80) continue;	//no good use yet
		
		int note_offset[4] = { 0 };
		int dist_2_offset[4] = { 12, 0, 48, 24 };
		int dist_4_smooth_offset[4] = { 12, 0, 24, 24 };
		int dist_4_buzzy_offset[4] = { 12, 0, 12, 24 };
		int dist_a_offset[4] = { 48, 24, 108, 24 };
		int dist_c_buzzy_offset[4] = { 24, 12, 84, 24 };
		int dist_c_gritty_offset[4] = { 12, 0, 72, 24 };
		int dist_c_unstable_offset[4] = { 36, 0, 96, 24 };

		bool IS_15KHZ = 0;
		bool IS_179MHZ = 0;
		bool IS_16BIT = 0;
		int dist_counter = 0;	//a shitty hack but who will call the police really?

		if (d == 0x40)
		{
			IS_SMOOTH_DIST_4 = 1;	//iteration 1: Smooth 
			IS_BUZZY_DIST_4 = 0;
			IS_UNSTABLE_DIST_4_1 = 0;
			IS_UNSTABLE_DIST_4_2 = 0;
		}
		if (d == 0xC0)
		{
			IS_BUZZY_DIST_C = 1;	//iteration 1: Buzzy
			IS_GRITTY_DIST_C = 0;
			IS_UNSTABLE_DIST_C = 0;
		}

repeat_dist:
		for (int c = 0; c < 4; c++)
		{
			if (d == 0x20) note_offset[c] = dist_2_offset[c];
			else if (d == 0x40) 
			{
				if (IS_SMOOTH_DIST_4) note_offset[c] = dist_4_smooth_offset[c];
				else if (IS_BUZZY_DIST_4) note_offset[c] = dist_4_buzzy_offset[c];
			}
			else if (d == 0xA0) note_offset[c] = dist_a_offset[c];
			else if (d == 0xC0)
			{
				if (IS_BUZZY_DIST_C) note_offset[c] = dist_c_buzzy_offset[c];
				else if (IS_GRITTY_DIST_C) note_offset[c] = dist_c_gritty_offset[c];
				else if (IS_UNSTABLE_DIST_C) note_offset[c] = dist_c_unstable_offset[c];
			}
			if (!note_offset[c]) continue;	//if no offset, no table should be created
			if (c == 0) { IS_15KHZ = 0; IS_179MHZ = 0; IS_16BIT = 0; }
			if (c == 1) { IS_15KHZ = 1; IS_179MHZ = 0; IS_16BIT = 0; }
			if (c == 2) { IS_15KHZ = 0; IS_179MHZ = 1; IS_16BIT = 0; }
			if (c == 3) { IS_15KHZ = 0; IS_179MHZ = 0; IS_16BIT = 1; }
			macro_table_gen(d, note_offset[c], IS_15KHZ, IS_179MHZ, IS_16BIT);
		}
		if (d == 0x40)
		{
			dist_counter++;	//the number of times the counter was used in the Distortion 4 case
			if (dist_counter == 1)
			{
				IS_SMOOTH_DIST_4 = 0;
				IS_BUZZY_DIST_4 = 1;		//iteration 2: Buzzy
				goto repeat_dist;
			}
			else IS_BUZZY_DIST_4 = 0;	//done all Distortion 4 modes
		} 
		if (d == 0xC0)
		{
			dist_counter++;	//the number of times the counter was used in the Distortion C case
			if (dist_counter == 1)
			{
				IS_BUZZY_DIST_C = 0;
				IS_GRITTY_DIST_C = 1;		//iteration 2: Gritty
				goto repeat_dist;
			}
			else if (dist_counter == 2)
			{
				IS_GRITTY_DIST_C = 0;
				IS_UNSTABLE_DIST_C = 1;		//iteration 3: Unstable
				goto repeat_dist;
			}
			else IS_UNSTABLE_DIST_C = 0;	//done all Distortion C modes
		}
	}
	for (int i = 0; i < 64; i++) //8-bit tables 
	{
		g_atarimem[RMT_FRQTABLES + 0x000 + i] = tab_64khz_2[i];
		g_atarimem[RMT_FRQTABLES + 0x040 + i] = tab_179mhz_2[i];
		g_atarimem[RMT_FRQTABLES + 0x200 + i] = tab_64khz_a_pure[i];
		g_atarimem[RMT_FRQTABLES + 0x240 + i] = tab_179mhz_a_pure[i];
		g_atarimem[RMT_FRQTABLES + 0x400 + i] = tab_64khz_c_gritty[i];
		g_atarimem[RMT_FRQTABLES + 0x440 + i] = tab_179mhz_c_gritty[i];
		g_atarimem[RMT_FRQTABLES + 0x580 + i] = tab_15khz_a_pure[i];
		g_atarimem[RMT_FRQTABLES + 0x5C0 + i] = tab_15khz_c_buzzy[i];
	}
	for (int i = 0; i < 52; i++) //8-bit tables (fill only the bytes that actually have valid pitches for each combinations) 
	{
		g_atarimem[RMT_FRQTABLES + 0x140 + i] = tab_179mhz_4_buzzy[i + 12];
		g_atarimem[RMT_FRQTABLES + 0x300 + i] = tab_64khz_c_gritty[i + 12];
		g_atarimem[RMT_FRQTABLES + 0x340 + i] = tab_179mhz_c_gritty[i + 12];
	}
	for (int i = 0; i < 64; i++) //8-bit tables (Distortion 4 1.79mhz Smooths)
	{
		if (tab_179mhz_4_smooth[i] == 0xFF) continue;	//no useful pitch
		g_atarimem[RMT_FRQTABLES + 0x140 + i] = tab_179mhz_4_smooth[i];
	}
	for (int i = 0; i < 64; i++) //8-bit tables (Distortion C 64khz Buzzies)
	{
		if (tab_64khz_c_buzzy[i] == 0xFF) continue;		//no useful pitch
		g_atarimem[RMT_FRQTABLES + 0x300 + i] = tab_64khz_c_buzzy[i];
	}
	for (int i = 0; i < 64; i++) //8-bit tables (Distortion C 1.79mhz Buzzies)
	{
		if (tab_179mhz_c_buzzy[i] == 0xFF) continue;	//no useful pitch
		g_atarimem[RMT_FRQTABLES + 0x340 + i] = tab_179mhz_c_buzzy[i];
	}
	for (int i = 0; i < 64; i++) //16-bit tables LSB
	{
		g_atarimem[RMT_FRQTABLES + 0x080 + (i * 2)] = tab_16bit_2[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x180 + (i * 2)] = tab_16bit_4_smooth[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x280 + (i * 2)] = tab_16bit_a_pure[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x380 + (i * 2)] = tab_16bit_c_buzzy[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x480 + (i * 2)] = tab_16bit_c_gritty[i * 2] & 0x00FF;
	}
	for (int i = 0; i < 64; i++) //16-bit tables MSB
	{
		g_atarimem[RMT_FRQTABLES + 0x080 + (i * 2) + 1] = tab_16bit_2[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x180 + (i * 2) + 1] = tab_16bit_4_smooth[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x280 + (i * 2) + 1] = tab_16bit_a_pure[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x380 + (i * 2) + 1] = tab_16bit_c_buzzy[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x480 + (i * 2) + 1] = tab_16bit_c_gritty[i * 2] >> 8;
	}
}
