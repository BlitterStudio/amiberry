 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Amiga keycodes
  *
  * (c) 1995 Bernd Schmidt
  */

/* First, two dummies */
#define AK_mousestuff 0x100
#define AK_inhibit 0x101
/* This mutates into AK_CTRL in keybuf.c. */
#define AK_RCTRL 0x7f

#define AK_A 0x20
#define AK_B 0x35
#define AK_C 0x33
#define AK_D 0x22
#define AK_E 0x12
#define AK_F 0x23
#define AK_G 0x24
#define AK_H 0x25
#define AK_I 0x17
#define AK_J 0x26
#define AK_K 0x27
#define AK_L 0x28
#define AK_M 0x37
#define AK_N 0x36
#define AK_O 0x18
#define AK_P 0x19
#define AK_Q 0x10
#define AK_R 0x13
#define AK_S 0x21
#define AK_T 0x14
#define AK_U 0x16
#define AK_V 0x34
#define AK_W 0x11
#define AK_X 0x32
#define AK_Y 0x15
#define AK_Z 0x31

#define AK_0 0x0A
#define AK_1 0x01
#define AK_2 0x02
#define AK_3 0x03
#define AK_4 0x04
#define AK_5 0x05
#define AK_6 0x06
#define AK_7 0x07
#define AK_8 0x08
#define AK_9 0x09

#define AK_NP0 0x0F
#define AK_NP1 0x1D
#define AK_NP2 0x1E
#define AK_NP3 0x1F
#define AK_NP4 0x2D
#define AK_NP5 0x2E
#define AK_NP6 0x2F
#define AK_NP7 0x3D
#define AK_NP8 0x3E
#define AK_NP9 0x3F

#define AK_NPDIV 0x5C
#define AK_NPMUL 0x5D
#define AK_NPSUB 0x4A
#define AK_NPADD 0x5E
#define AK_NPDEL 0x3C
#define AK_NPLPAREN 0x5A
#define AK_NPRPAREN 0x5B

#define AK_F1 0x50
#define AK_F2 0x51
#define AK_F3 0x52
#define AK_F4 0x53
#define AK_F5 0x54
#define AK_F6 0x55
#define AK_F7 0x56
#define AK_F8 0x57
#define AK_F9 0x58
#define AK_F10 0x59

#define AK_UP 0x4C
#define AK_DN 0x4D
#define AK_LF 0x4F
#define AK_RT 0x4E

#define AK_SPC 0x40
#define AK_BS 0x41
#define AK_TAB 0x42
#define AK_ENT 0x43
#define AK_RET 0x44
#define AK_ESC 0x45
#define AK_DEL 0x46

#define AK_LSH 0x60
#define AK_RSH 0x61
#define AK_CAPSLOCK 0x62
#define AK_CTRL 0x63
#define AK_LALT 0x64
#define AK_RALT 0x65
#define AK_LAMI 0x66
#define AK_RAMI 0x67
#define AK_HELP 0x5F

/* The following have different mappings on national keyboards */

#define AK_LBRACKET 0x1A
#define AK_RBRACKET 0x1B
#define AK_SEMICOLON 0x29
#define AK_COMMA 0x38
#define AK_PERIOD 0x39
#define AK_SLASH 0x3A
#define AK_BACKSLASH 0x0D
#define AK_QUOTE 0x2A
#define AK_NUMBERSIGN 0x2B
#define AK_LTGT 0x30
#define AK_BACKQUOTE 0x00
#define AK_MINUS 0x0B
#define AK_EQUAL 0x0C
#define AK_RESETWARNING 0x78
#define AK_INIT_POWERUP 0xfd
#define AK_TERM_POWERUP 0xfe
