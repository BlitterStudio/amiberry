#ifndef UAE_OD_FS_MIDI_H
#define UAE_OD_FS_MIDI_H

int midi_open(void);
void midi_close(void);
void midi_send_byte(uint8_t ch);
int midi_has_byte(void);
int midi_recv_byte(uint8_t *ch);

// Aliases for compatibility with WinUAE

typedef enum
{
	midi_input,
	midi_output
} midi_direction_e;


static int midi_ready = 0;

extern int Midi_Parse (midi_direction_e direction, uint8_t *c);
extern int Midi_Open(void);
extern void Midi_Close(void);
extern void Midi_Reopen(void);

#endif
