#ifndef CFGFILE_H_
#define CFGFILE_H_

extern char *cfgfile_subst_path (const char *path, const char *subst, const char *file);
extern char * make_hard_dir_cfg_line (char *src, char *dst);
extern char * make_hard_file_cfg_line (char *src, char *dst);
extern void parse_filesys_spec (int readonly, char *spec);
extern void parse_hardfile_spec (char *spec);

#endif
