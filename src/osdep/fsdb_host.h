#pragma once

typedef struct fsdb_file_info {
    int type;
    uint32_t mode;
    int days;
    int mins;
    int ticks;
    char* comment;

} fsdb_file_info;

struct fs_time_val {
    int32_t tv_sec;
    int32_t tv_usec;
};
typedef struct fs_time_val fs_time_val;

struct fs_stat {
    int mode;
    int64_t size;
    time_t atime;
    time_t mtime;
    time_t ctime;
    int atime_nsec;
    int mtime_nsec;
    int ctime_nsec;
    int64_t blocks;
};

void fsdb_init_file_info(fsdb_file_info* info);
int fsdb_set_file_info(const char* nname, fsdb_file_info* info);

extern bool fs_path_exists(const std::string& s);
extern int g_fsdb_debug;
extern int my_errno;
extern int host_errno_to_dos_errno(int err);
extern int fs_stat(const char* path, struct fs_stat* buf);
extern char* Utf8ToLatin1String(char* s);
extern std::string iso_8859_1_to_utf8(std::string& str);
extern string prefix_with_application_directory_path(string currentpath);