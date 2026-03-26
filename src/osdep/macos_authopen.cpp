#include "macos_authopen.h"

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "filesys.h"

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Darwin macOS raw-disk authopen wrapper (uses public filesystem APIs).
// Gate raw-disk access to devices enumerated by the public filesys API.
// We only allow /dev/disk* or /dev/rdisk* that exactly match an enumerated
// harddrive path and that are not mounted.
int macos_authopen_fd(const char* path, int open_flags, std::string& error) {
    error.clear();
    if (path == nullptr || *path == '\0') {
        error = "invalid path";
        return -1;
    }
    if (open_flags < 0) {
        error = "invalid flags";
        return -1;
    }

    // Quick allowlist: require the path to start with /dev/disk or /dev/rdisk
    const char* p = path;
    bool is_rawdisk_like = (strncmp(p, "/dev/disk", 9) == 0) || (strncmp(p, "/dev/rdisk", 10) == 0);
    if (!is_rawdisk_like) {
        error = "not eligible";
        return -1;
    }

    // Verify exact path against public drive enumeration (do not access internal uaedrives).
    bool exact_match_found = false;
    int ndrives = hdf_getnumharddrives();
    for (int idx = 0; idx < ndrives; ++idx) {
        const char* enumerated_path = hdf_getpathharddrive(idx);
        if (enumerated_path) {
            if (strcmp(enumerated_path, path) == 0) {
                int sectorsize = 0;
                int dangerous = 0;
                uae_u32 outflags = 0;
                char* drvname = (char*)hdf_getnameharddrive(idx, 0, &sectorsize, &dangerous, &outflags);
                if (drvname) free(drvname);
                if (outflags & HDF_DRIVEFLAG_MOUNTED) {
                    error = "mounted";
                    free((void*)enumerated_path);
                    return -1;
                }
                exact_match_found = true;
                free((void*)enumerated_path);
                break;
            }
        }
        // Non-matching path: free the allocated string
        if (enumerated_path) {
            free((void*)enumerated_path);
        }
    }
    if (!exact_match_found) {
        error = "not eligible";
        return -1;
    }

    char flags_buf[32];
    std::snprintf(flags_buf, sizeof(flags_buf), "%d", open_flags);

    int sv[2] = {-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        error = "socketpair failed";
        return -1;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(sv[0]);
        close(sv[1]);
        error = "fork failed";
        return -1;
    }

    if (pid == 0) {
        close(sv[0]);
        if (dup2(sv[1], STDOUT_FILENO) < 0) {
            _exit(127);
        }
        if (sv[1] != STDOUT_FILENO) {
            close(sv[1]);
        }
        execl("/usr/libexec/authopen", "authopen", "-stdoutpipe", "-o", flags_buf, path, static_cast<char*>(nullptr));
        _exit(127);
    }

    close(sv[1]);

    int received_fd = -1;
    char payload = 0;
    char cmsg_buf[CMSG_SPACE(sizeof(int))] = {};
    iovec iov{};
    iov.iov_base = &payload;
    iov.iov_len = sizeof(payload);

    msghdr msg {};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    const ssize_t recv_len = recvmsg(sv[0], &msg, 0);
    if (recv_len >= 0 && (msg.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) == 0) {
        for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS && cmsg->cmsg_len >= CMSG_LEN(sizeof(int))) {
                std::memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(received_fd));
                break;
            }
        }
    }
    close(sv[0]);

    int status = 0;
    pid_t waited = -1;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        if (received_fd >= 0) {
            close(received_fd);
        }
        error = "waitpid failed";
        return -1;
    }

    if (recv_len < 0) {
        error = "recvmsg failed";
        return -1;
    }
    if (received_fd < 0) {
        error = "missing fd";
        return -1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        close(received_fd);
        error = "authopen failed";
        return -1;
    }

    error.clear();
    return received_fd;
}
