#include "sysconfig.h"
#include "sysdeps.h"

#include "uae/cdrom.h"

/* CDROM MODE 1 EDC/ECC code (from Reed-Solomon library by Heiko Eissfeldt) */

/*****************************************************************/
/*                                                               */
/* CRC LOOKUP TABLE                                              */
/* ================                                              */
/* The following CRC lookup table was generated automagically    */
/* by the Rocksoft^tm Model CRC Algorithm Table Generation       */
/* Program V1.0 using the following model parameters:            */
/*                                                               */
/*    Width   : 4 bytes.                                         */
/*    Poly    : 0x8001801BL                                      */
/*    Reverse : TRUE.                                            */
/*                                                               */
/* For more information on the Rocksoft^tm Model CRC Algorithm,  */
/* see the document titled "A Painless Guide to CRC Error        */
/* Detection Algorithms" by Ross Williams                        */
/* (ross@guest.adelaide.edu.au.). This document is likely to be  */
/* in the FTP archive "ftp.adelaide.edu.au/pub/rocksoft".        */
/*                                                               */
/*****************************************************************/

static const uae_u32 EDC_crctable[256] =
{
	0x00000000L, 0x90910101L, 0x91210201L, 0x01B00300L,
	0x92410401L, 0x02D00500L, 0x03600600L, 0x93F10701L,
	0x94810801L, 0x04100900L, 0x05A00A00L, 0x95310B01L,
	0x06C00C00L, 0x96510D01L, 0x97E10E01L, 0x07700F00L,
	0x99011001L, 0x09901100L, 0x08201200L, 0x98B11301L,
	0x0B401400L, 0x9BD11501L, 0x9A611601L, 0x0AF01700L,
	0x0D801800L, 0x9D111901L, 0x9CA11A01L, 0x0C301B00L,
	0x9FC11C01L, 0x0F501D00L, 0x0EE01E00L, 0x9E711F01L,
	0x82012001L, 0x12902100L, 0x13202200L, 0x83B12301L,
	0x10402400L, 0x80D12501L, 0x81612601L, 0x11F02700L,
	0x16802800L, 0x86112901L, 0x87A12A01L, 0x17302B00L,
	0x84C12C01L, 0x14502D00L, 0x15E02E00L, 0x85712F01L,
	0x1B003000L, 0x8B913101L, 0x8A213201L, 0x1AB03300L,
	0x89413401L, 0x19D03500L, 0x18603600L, 0x88F13701L,
	0x8F813801L, 0x1F103900L, 0x1EA03A00L, 0x8E313B01L,
	0x1DC03C00L, 0x8D513D01L, 0x8CE13E01L, 0x1C703F00L,
	0xB4014001L, 0x24904100L, 0x25204200L, 0xB5B14301L,
	0x26404400L, 0xB6D14501L, 0xB7614601L, 0x27F04700L,
	0x20804800L, 0xB0114901L, 0xB1A14A01L, 0x21304B00L,
	0xB2C14C01L, 0x22504D00L, 0x23E04E00L, 0xB3714F01L,
	0x2D005000L, 0xBD915101L, 0xBC215201L, 0x2CB05300L,
	0xBF415401L, 0x2FD05500L, 0x2E605600L, 0xBEF15701L,
	0xB9815801L, 0x29105900L, 0x28A05A00L, 0xB8315B01L,
	0x2BC05C00L, 0xBB515D01L, 0xBAE15E01L, 0x2A705F00L,
	0x36006000L, 0xA6916101L, 0xA7216201L, 0x37B06300L,
	0xA4416401L, 0x34D06500L, 0x35606600L, 0xA5F16701L,
	0xA2816801L, 0x32106900L, 0x33A06A00L, 0xA3316B01L,
	0x30C06C00L, 0xA0516D01L, 0xA1E16E01L, 0x31706F00L,
	0xAF017001L, 0x3F907100L, 0x3E207200L, 0xAEB17301L,
	0x3D407400L, 0xADD17501L, 0xAC617601L, 0x3CF07700L,
	0x3B807800L, 0xAB117901L, 0xAAA17A01L, 0x3A307B00L,
	0xA9C17C01L, 0x39507D00L, 0x38E07E00L, 0xA8717F01L,
	0xD8018001L, 0x48908100L, 0x49208200L, 0xD9B18301L,
	0x4A408400L, 0xDAD18501L, 0xDB618601L, 0x4BF08700L,
	0x4C808800L, 0xDC118901L, 0xDDA18A01L, 0x4D308B00L,
	0xDEC18C01L, 0x4E508D00L, 0x4FE08E00L, 0xDF718F01L,
	0x41009000L, 0xD1919101L, 0xD0219201L, 0x40B09300L,
	0xD3419401L, 0x43D09500L, 0x42609600L, 0xD2F19701L,
	0xD5819801L, 0x45109900L, 0x44A09A00L, 0xD4319B01L,
	0x47C09C00L, 0xD7519D01L, 0xD6E19E01L, 0x46709F00L,
	0x5A00A000L, 0xCA91A101L, 0xCB21A201L, 0x5BB0A300L,
	0xC841A401L, 0x58D0A500L, 0x5960A600L, 0xC9F1A701L,
	0xCE81A801L, 0x5E10A900L, 0x5FA0AA00L, 0xCF31AB01L,
	0x5CC0AC00L, 0xCC51AD01L, 0xCDE1AE01L, 0x5D70AF00L,
	0xC301B001L, 0x5390B100L, 0x5220B200L, 0xC2B1B301L,
	0x5140B400L, 0xC1D1B501L, 0xC061B601L, 0x50F0B700L,
	0x5780B800L, 0xC711B901L, 0xC6A1BA01L, 0x5630BB00L,
	0xC5C1BC01L, 0x5550BD00L, 0x54E0BE00L, 0xC471BF01L,
	0x6C00C000L, 0xFC91C101L, 0xFD21C201L, 0x6DB0C300L,
	0xFE41C401L, 0x6ED0C500L, 0x6F60C600L, 0xFFF1C701L,
	0xF881C801L, 0x6810C900L, 0x69A0CA00L, 0xF931CB01L,
	0x6AC0CC00L, 0xFA51CD01L, 0xFBE1CE01L, 0x6B70CF00L,
	0xF501D001L, 0x6590D100L, 0x6420D200L, 0xF4B1D301L,
	0x6740D400L, 0xF7D1D501L, 0xF661D601L, 0x66F0D700L,
	0x6180D800L, 0xF111D901L, 0xF0A1DA01L, 0x6030DB00L,
	0xF3C1DC01L, 0x6350DD00L, 0x62E0DE00L, 0xF271DF01L,
	0xEE01E001L, 0x7E90E100L, 0x7F20E200L, 0xEFB1E301L,
	0x7C40E400L, 0xECD1E501L, 0xED61E601L, 0x7DF0E700L,
	0x7A80E800L, 0xEA11E901L, 0xEBA1EA01L, 0x7B30EB00L,
	0xE8C1EC01L, 0x7850ED00L, 0x79E0EE00L, 0xE971EF01L,
	0x7700F000L, 0xE791F101L, 0xE621F201L, 0x76B0F300L,
	0xE541F401L, 0x75D0F500L, 0x7460F600L, 0xE4F1F701L,
	0xE381F801L, 0x7310F900L, 0x72A0FA00L, 0xE231FB01L,
	0x71C0FC00L, 0xE151FD01L, 0xE0E1FE01L, 0x7070FF00L
};

/*****************************************************************/
/*                   End of CRC Lookup Table                     */
/*****************************************************************/

static uae_u8 rs_l12_alog[255] = {
	1, 2, 4, 8,16,32,64,128,29,58,116,232,205,135,19,38,76,152,45,90,180,117,234,201,143, 3, 6,12,24,48,96,192,157,39,78,156,37,74,148,53,106,212,181,119,238,193,159,35,70,140, 5,10,20,40,80,160,93,186,105,210,185,111,222,161,95,190,97,194,153,47,94,188,101,202,137,15,30,60,120,240,253,231,211,187,107,214,177,127,254,225,223,163,91,182,113,226,217,175,67,134,17,34,68,136,13,26,52,104,208,189,103,206,129,31,62,124,248,237,199,147,59,118,236,197,151,51,102,204,133,23,46,92,184,109,218,169,79,158,33,66,132,21,42,84,168,77,154,41,82,164,85,170,73,146,57,114,228,213,183,115,230,209,191,99,198,145,63,126,252,229,215,179,123,246,241,255,227,219,171,75,150,49,98,196,149,55,110,220,165,87,174,65,130,25,50,100,200,141, 7,14,28,56,112,224,221,167,83,166,81,162,89,178,121,242,249,239,195,155,43,86,172,69,138, 9,18,36,72,144,61,122,244,245,247,243,251,235,203,139,11,22,44,88,176,125,250,233,207,131,27,54,108,216,173,71,142,};
	static uae_u8 rs_l12_log[256] = {
		0, 0, 1,25, 2,50,26,198, 3,223,51,238,27,104,199,75, 4,100,224,14,52,141,239,129,28,193,105,248,200, 8,76,113, 5,138,101,47,225,36,15,33,53,147,142,218,240,18,130,69,29,181,194,125,106,39,249,185,201,154, 9,120,77,228,114,166, 6,191,139,98,102,221,48,253,226,152,37,179,16,145,34,136,54,208,148,206,143,150,219,189,241,210,19,92,131,56,70,64,30,66,182,163,195,72,126,110,107,58,40,84,250,133,186,61,202,94,155,159,10,21,121,43,78,212,229,172,115,243,167,87, 7,112,192,247,140,128,99,13,103,74,222,237,49,197,254,24,227,165,153,119,38,184,180,124,17,68,146,217,35,32,137,46,55,63,209,91,149,188,207,205,144,135,151,178,220,252,190,97,242,86,211,171,20,42,93,158,132,60,57,83,71,109,65,162,31,45,67,216,183,123,164,118,196,23,73,236,127,12,111,246,108,161,59,82,41,157,85,170,251,96,134,177,187,204,62,90,203,89,95,176,156,169,160,81,11,245,22,235,122,117,44,215,79,174,213,233,230,231,173,232,116,214,244,234,168,80,88,175,};
		static uae_u8 DQ[2][43] = {
			{190,96,250,132,59,81,159,154,200,7,111,245,10,20,41,156,168,79,173,231,229,171,210,240,17,67,215,43,120,8,199,74,102,220,251,95,175,87,166,113,75,198,25,},
			{97,251,133,60,82,160,155,201,8,112,246,11,21,42,157,169,80,174,232,230,172,211,241,18,68,216,44,121,9,200,75,103,221,252,96,176,88,167,114,76,199,26,1,},
		};
		static uae_u8 DP[2][24] = {
			{231,229,171,210,240,17,67,215,43,120,8,199,74,102,220,251,95,175,87,166,113,75,198,25,},
			{230,172,211,241,18,68,216,44,121,9,200,75,103,221,252,96,176,88,167,114,76,199,26,1,},
		};

		/* data sector definitions for RSPC */
		/* user data bytes per frame */
#define L2_RAW (1024*2)
		/* parity bytes for 16 bit units */
#define L2_Q   (26*2*2)
#define L2_P   (43*2*2)
#define RS_L12_BITS 8

		static uae_u32 build_edc (const uae_u8 *inout, int from, int upto)
		{
			const uae_u8 *p = inout + from;
			uae_u32 result = 0;
			for (; from <= upto; from++)
				result = EDC_crctable[(result ^ *p++) & 0xff] ^ (result >> 8);
			return result;
		}

		static void encode_L2_Q(uae_u8 *inout)
		{
			uae_u8 *Q;
			int i,j;

			Q = inout + 4 + L2_RAW + 4 + 8 + L2_P;
			memset(Q, 0, L2_Q);
			for (j = 0; j < 26; j++) {
				for (i = 0; i < 43; i++) {
					uae_u8 data;
					/* LSB */
					data = inout[(j*43*2+i*2*44) % (4 + L2_RAW + 4 + 8 + L2_P)];
					if (data != 0) {
						uae_u32 base = rs_l12_log[data];
						uae_u32 sum = base + DQ[0][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						Q[0]    ^= rs_l12_alog[sum];
						sum = base + DQ[1][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						Q[26*2] ^= rs_l12_alog[sum];
					}
					/* MSB */
					data = inout[(j*43*2+i*2*44+1) % (4 + L2_RAW + 4 + 8 + L2_P)];
					if (data != 0) {
						uae_u32 base = rs_l12_log[data];
						uae_u32 sum = base+DQ[0][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						Q[1]      ^= rs_l12_alog[sum];
						sum = base + DQ[1][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						Q[26*2+1] ^= rs_l12_alog[sum];
					}
				}
				Q += 2;
			}
		}

		static void encode_L2_P(uae_u8 inout[4 + L2_RAW + 4 + 8 + L2_P])
		{
			uae_u8 *P;
			int i,j;

			P = inout + 4 + L2_RAW + 4 + 8;
			memset(P, 0, L2_P);
			for (j = 0; j < 43; j++) {
				for (i = 0; i < 24; i++) {
					uae_u8 data;
					/* LSB */
					data = inout[i*2*43];
					if (data != 0) {
						uae_u32 base = rs_l12_log[data];
						uae_u32 sum = base + DP[0][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						P[0]    ^= rs_l12_alog[sum];
						sum = base + DP[1][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						P[43*2] ^= rs_l12_alog[sum];
					}
					/* MSB */
					data = inout[i*2*43+1];
					if (data != 0) {
						uae_u32 base = rs_l12_log[data];
						uae_u32 sum = base + DP[0][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						P[1]      ^= rs_l12_alog[sum];
						sum = base + DP[1][i];
						if (sum >= ((1 << RS_L12_BITS)-1))
							sum -= (1 << RS_L12_BITS)-1;
						P[43*2+1] ^= rs_l12_alog[sum];
					}
				}
				P += 2;
				inout += 2;
			}
		}

		static uae_u8 tobcd (uae_u8 v)
		{
			return ((v / 10) << 4) | (v % 10);
		}

		void encode_l2 (uae_u8 *p, int address)
		{
			uae_u32 v;

			p[0] = 0x00;
			memset (p + 1, 0xff, 11);
			p[12] = tobcd ((uae_u8)(address / (60 * 75)));
			p[13] = tobcd ((uae_u8)((address / 75) % 60));
			p[14] = tobcd ((uae_u8)(address % 75));
			p[15] = 1; /* MODE1 */
			v = build_edc (p, 0, 16 + 2048 - 1);
			p[2064 + 0] = (uae_u8) (v >> 0);
			p[2064 + 1] = (uae_u8) (v >> 8);
			p[2064 + 2] = (uae_u8) (v >> 16);
			p[2064 + 3] = (uae_u8) (v >> 24);
			memset (p + 2064 + 4, 0, 8);
			encode_L2_P (p + 12);
			encode_L2_Q (p + 12);
		}
