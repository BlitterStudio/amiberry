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

extern int Midi_Parse (midi_direction_e direction, uint8_t *c);
int Midi_Open(void);
void Midi_Close(void);

#endif
