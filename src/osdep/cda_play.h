class cda_audio {
private:
  int bufsize;
	int sectorsize;
  int volume[2];
  bool playing;
  bool active;

public:
  uae_u8 *buffers[2];
  int currBuf;
  int num_sectors;

	cda_audio(int num_sectors, int sectorsize, int samplerate);
  ~cda_audio();
  void setvolume(int left, int right);
  bool play(int bufnum);
  void wait(void);
  void wait(int bufnum);
	bool isplaying(int bufnum);
};
