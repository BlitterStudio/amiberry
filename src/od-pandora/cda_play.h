class cda_audio {
private:
  int bufsize;
  int volume[2];
  bool playing;
  bool active;

public:
  uae_u8 *buffers[2];
  int num_sectors;

  cda_audio(int num_sectors);
  ~cda_audio();
  void setvolume(int master, int left, int right);
  bool play(int bufnum);
  void wait(void);
  void wait(int bufnum);
};
