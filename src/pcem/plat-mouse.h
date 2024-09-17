#ifdef __cplusplus
extern "C" {
#endif
#ifndef UAE

        void mouse_init();
        void mouse_close();
        extern int mouse_buttons;
        void mouse_poll_host();
        void mouse_get_mickeys(int *x, int *y, int *z);
	extern int mousecapture;
#endif
#ifdef __cplusplus
}
#endif
#ifdef UAE
extern int mouse_buttons;
#endif
