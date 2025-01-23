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

extern bool fs_path_exists(const std::string& s);
extern std::string iso_8859_1_to_utf8(const std::string& str);
extern void utf8_to_latin1_string(std::string& input, std::string& output);
extern std::string prefix_with_application_directory_path(std::string currentpath);
extern std::string prefix_with_data_path(const std::string& filename);
extern std::string prefix_with_whdboot_path(const std::string& filename);
