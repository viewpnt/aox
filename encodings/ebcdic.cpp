// Copyright Arnt Gulbrandsen <arnt@gulbrandsen.priv.no>.

// Copyright 2009 The Archiveopteryx Developers <info@aox.org>

#include "ebcdic.h"

static const uint ebcdictable[256] = {
    0x0000, // 0 -> U+0000
    0x0001, // 1 -> U+0001
    0x0002, // 2 -> U+0002
    0x0003, // 3 -> U+0003
    0xFFFD, // 4 -> U+FFFD
    0x0009, // 5 -> U+0009
    0xFFFD, // 6 -> U+FFFD
    0x007F, // 7 -> U+007F
    0xFFFD, // 8 -> U+FFFD
    0xFFFD, // 9 -> U+FFFD
    0xFFFD, // 10 -> U+FFFD
    0x000B, // 11 -> U+000B
    0x000C, // 12 -> U+000C
    0x000D, // 13 -> U+000D
    0x000E, // 14 -> U+000E
    0x000F, // 15 -> U+000F
    0x0010, // 16 -> U+0010
    0x0011, // 17 -> U+0011
    0x0012, // 18 -> U+0012
    0x0013, // 19 -> U+0013
    0xFFFD, // 20 -> U+FFFD
    0x0085, // 21 -> U+0085
    0x0008, // 22 -> U+0008
    0xFFFD, // 23 -> U+FFFD
    0x0018, // 24 -> U+0018
    0x0019, // 25 -> U+0019
    0xFFFD, // 26 -> U+FFFD
    0xFFFD, // 27 -> U+FFFD
    0x001C, // 28 -> U+001C
    0x001D, // 29 -> U+001D
    0x001E, // 30 -> U+001E
    0x001F, // 31 -> U+001F
    0xFFFD, // 32 -> U+FFFD
    0xFFFD, // 33 -> U+FFFD
    0xFFFD, // 34 -> U+FFFD
    0xFFFD, // 35 -> U+FFFD
    0xFFFD, // 36 -> U+FFFD
    0x000A, // 37 -> U+000A
    0x0017, // 38 -> U+0017
    0x001B, // 39 -> U+001B
    0xFFFD, // 40 -> U+FFFD
    0xFFFD, // 41 -> U+FFFD
    0xFFFD, // 42 -> U+FFFD
    0xFFFD, // 43 -> U+FFFD
    0xFFFD, // 44 -> U+FFFD
    0x0005, // 45 -> U+0005
    0x0006, // 46 -> U+0006
    0x0007, // 47 -> U+0007
    0xFFFD, // 48 -> U+FFFD
    0xFFFD, // 49 -> U+FFFD
    0x0016, // 50 -> U+0016
    0xFFFD, // 51 -> U+FFFD
    0xFFFD, // 52 -> U+FFFD
    0xFFFD, // 53 -> U+FFFD
    0xFFFD, // 54 -> U+FFFD
    0x0004, // 55 -> U+0004
    0xFFFD, // 56 -> U+FFFD
    0xFFFD, // 57 -> U+FFFD
    0xFFFD, // 58 -> U+FFFD
    0xFFFD, // 59 -> U+FFFD
    0x0014, // 60 -> U+0014
    0x0015, // 61 -> U+0015
    0xFFFD, // 62 -> U+FFFD
    0x001A, // 63 -> U+001A
    0x0020, // 64 -> U+0020
    0x00A0, // 65 -> U+00A0
    0xFFFD, // 66 -> U+FFFD
    0xFFFD, // 67 -> U+FFFD
    0xFFFD, // 68 -> U+FFFD
    0xFFFD, // 69 -> U+FFFD
    0xFFFD, // 70 -> U+FFFD
    0xFFFD, // 71 -> U+FFFD
    0xFFFD, // 72 -> U+FFFD
    0xFFFD, // 73 -> U+FFFD
    0xFFFD, // 74 -> U+FFFD
    0x002E, // 75 -> U+002E
    0x003C, // 76 -> U+003C
    0x0028, // 77 -> U+0028
    0x002B, // 78 -> U+002B
    0x007C, // 79 -> U+007C
    0x0026, // 80 -> U+0026
    0xFFFD, // 81 -> U+FFFD
    0xFFFD, // 82 -> U+FFFD
    0xFFFD, // 83 -> U+FFFD
    0xFFFD, // 84 -> U+FFFD
    0xFFFD, // 85 -> U+FFFD
    0xFFFD, // 86 -> U+FFFD
    0xFFFD, // 87 -> U+FFFD
    0xFFFD, // 88 -> U+FFFD
    0xFFFD, // 89 -> U+FFFD
    0x0021, // 90 -> U+0021
    0x0024, // 91 -> U+0024
    0x002A, // 92 -> U+002A
    0x0029, // 93 -> U+0029
    0x003B, // 94 -> U+003B
    0x00AC, // 95 -> U+00AC
    0x002D, // 96 -> U+002D
    0x002F, // 97 -> U+002F
    0xFFFD, // 98 -> U+FFFD
    0xFFFD, // 99 -> U+FFFD
    0xFFFD, // 100 -> U+FFFD
    0xFFFD, // 101 -> U+FFFD
    0xFFFD, // 102 -> U+FFFD
    0xFFFD, // 103 -> U+FFFD
    0xFFFD, // 104 -> U+FFFD
    0xFFFD, // 105 -> U+FFFD
    0x00A6, // 106 -> U+00A6
    0x002C, // 107 -> U+002C
    0x0025, // 108 -> U+0025
    0x005F, // 109 -> U+005F
    0x003E, // 110 -> U+003E
    0x003F, // 111 -> U+003F
    0xFFFD, // 112 -> U+FFFD
    0xFFFD, // 113 -> U+FFFD
    0xFFFD, // 114 -> U+FFFD
    0xFFFD, // 115 -> U+FFFD
    0xFFFD, // 116 -> U+FFFD
    0xFFFD, // 117 -> U+FFFD
    0xFFFD, // 118 -> U+FFFD
    0xFFFD, // 119 -> U+FFFD
    0xFFFD, // 120 -> U+FFFD
    0x0060, // 121 -> U+0060
    0x003A, // 122 -> U+003A
    0x0023, // 123 -> U+0023
    0x0040, // 124 -> U+0040
    0x0027, // 125 -> U+0027
    0x003D, // 126 -> U+003D
    0x0022, // 127 -> U+0022
    0xFFFD, // 128 -> U+FFFD
    0x0061, // 129 -> U+0061
    0x0062, // 130 -> U+0062
    0x0063, // 131 -> U+0063
    0x0064, // 132 -> U+0064
    0x0065, // 133 -> U+0065
    0x0066, // 134 -> U+0066
    0x0067, // 135 -> U+0067
    0x0068, // 136 -> U+0068
    0x0069, // 137 -> U+0069
    0xFFFD, // 138 -> U+FFFD
    0xFFFD, // 139 -> U+FFFD
    0xFFFD, // 140 -> U+FFFD
    0xFFFD, // 141 -> U+FFFD
    0xFFFD, // 142 -> U+FFFD
    0x00B1, // 143 -> U+00B1
    0xFFFD, // 144 -> U+FFFD
    0x006A, // 145 -> U+006A
    0x006B, // 146 -> U+006B
    0x006C, // 147 -> U+006C
    0x006D, // 148 -> U+006D
    0x006E, // 149 -> U+006E
    0x006F, // 150 -> U+006F
    0x0070, // 151 -> U+0070
    0x0071, // 152 -> U+0071
    0x0072, // 153 -> U+0072
    0xFFFD, // 154 -> U+FFFD
    0xFFFD, // 155 -> U+FFFD
    0xFFFD, // 156 -> U+FFFD
    0xFFFD, // 157 -> U+FFFD
    0xFFFD, // 158 -> U+FFFD
    0xFFFD, // 159 -> U+FFFD
    0xFFFD, // 160 -> U+FFFD
    0x007E, // 161 -> U+007E
    0x0073, // 162 -> U+0073
    0x0074, // 163 -> U+0074
    0x0075, // 164 -> U+0075
    0x0076, // 165 -> U+0076
    0x0077, // 166 -> U+0077
    0x0078, // 167 -> U+0078
    0x0079, // 168 -> U+0079
    0x007A, // 169 -> U+007A
    0xFFFD, // 170 -> U+FFFD
    0xFFFD, // 171 -> U+FFFD
    0xFFFD, // 172 -> U+FFFD
    0xFFFD, // 173 -> U+FFFD
    0xFFFD, // 174 -> U+FFFD
    0xFFFD, // 175 -> U+FFFD
    0x005E, // 176 -> U+005E
    0xFFFD, // 177 -> U+FFFD
    0xFFFD, // 178 -> U+FFFD
    0xFFFD, // 179 -> U+FFFD
    0xFFFD, // 180 -> U+FFFD
    0xFFFD, // 181 -> U+FFFD
    0xFFFD, // 182 -> U+FFFD
    0xFFFD, // 183 -> U+FFFD
    0xFFFD, // 184 -> U+FFFD
    0xFFFD, // 185 -> U+FFFD
    0x005B, // 186 -> U+005B
    0x005D, // 187 -> U+005D
    0xFFFD, // 188 -> U+FFFD
    0xFFFD, // 189 -> U+FFFD
    0xFFFD, // 190 -> U+FFFD
    0xFFFD, // 191 -> U+FFFD
    0x007B, // 192 -> U+007B
    0x0041, // 193 -> U+0041
    0x0042, // 194 -> U+0042
    0x0043, // 195 -> U+0043
    0x0044, // 196 -> U+0044
    0x0045, // 197 -> U+0045
    0x0046, // 198 -> U+0046
    0x0047, // 199 -> U+0047
    0x0048, // 200 -> U+0048
    0x0049, // 201 -> U+0049
    0x00AD, // 202 -> U+00AD
    0xFFFD, // 203 -> U+FFFD
    0xFFFD, // 204 -> U+FFFD
    0xFFFD, // 205 -> U+FFFD
    0xFFFD, // 206 -> U+FFFD
    0xFFFD, // 207 -> U+FFFD
    0x007D, // 208 -> U+007D
    0x004A, // 209 -> U+004A
    0x004B, // 210 -> U+004B
    0x004C, // 211 -> U+004C
    0x004D, // 212 -> U+004D
    0x004E, // 213 -> U+004E
    0x004F, // 214 -> U+004F
    0x0050, // 215 -> U+0050
    0x0051, // 216 -> U+0051
    0x0052, // 217 -> U+0052
    0xFFFD, // 218 -> U+FFFD
    0xFFFD, // 219 -> U+FFFD
    0xFFFD, // 220 -> U+FFFD
    0xFFFD, // 221 -> U+FFFD
    0xFFFD, // 222 -> U+FFFD
    0xFFFD, // 223 -> U+FFFD
    0x005C, // 224 -> U+005C
    0xFFFD, // 225 -> U+FFFD
    0x0053, // 226 -> U+0053
    0x0054, // 227 -> U+0054
    0x0055, // 228 -> U+0055
    0x0056, // 229 -> U+0056
    0x0057, // 230 -> U+0057
    0x0058, // 231 -> U+0058
    0x0059, // 232 -> U+0059
    0x005A, // 233 -> U+005A
    0xFFFD, // 234 -> U+FFFD
    0xFFFD, // 235 -> U+FFFD
    0xFFFD, // 236 -> U+FFFD
    0xFFFD, // 237 -> U+FFFD
    0xFFFD, // 238 -> U+FFFD
    0xFFFD, // 239 -> U+FFFD
    0x0030, // 240 -> U+0030
    0x0031, // 241 -> U+0031
    0x0032, // 242 -> U+0032
    0x0033, // 243 -> U+0033
    0x0034, // 244 -> U+0034
    0x0035, // 245 -> U+0035
    0x0036, // 246 -> U+0036
    0x0037, // 247 -> U+0037
    0x0038, // 248 -> U+0038
    0x0039, // 249 -> U+0039
    0xFFFD, // 250 -> U+FFFD
    0xFFFD, // 251 -> U+FFFD
    0xFFFD, // 252 -> U+FFFD
    0xFFFD, // 253 -> U+FFFD
    0xFFFD, // 254 -> U+FFFD
    0xFFFD // 255 -> U+FFFD
};

/*! \class EbcdicCodec ebcdic.h

    The EbcdicCodec class implements EBCDIC as implemented by recode
    and iconv. Nobody should use EBCDIC.
    
    Our table is based on http://en.wikipedia.org/wiki/EBCDIC
*/


/*!  Constructs a codec for the EBCDIC character set/encoding. */

EbcdicCodec::EbcdicCodec()
    : TableCodec( ebcdictable, "ebcdic" )
{
}

//codec ebcdic EbcdicCodec
