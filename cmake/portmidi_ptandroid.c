/* ptandroid.c -- minimal PortTime backend for Android
 *
 * PortMidi's upstream ptlinux.c depends on sys/timeb.h which is not available
 * in the Android NDK. Amiberry needs PortMidi built and linked on Android,
 * but does not require high-precision PortTime scheduling.
 *
 * This file implements the PortTime API using a simple monotonic clock and
 * a dedicated callback thread.
 */

#include "porttime.h"

#include <stddef.h>
#include <stdint.h>

#include <pthread.h>
#include <time.h>
#include <unistd.h>

static pthread_t pt_thread;
static pthread_mutex_t pt_mutex = PTHREAD_MUTEX_INITIALIZER;
static int pt_running = 0;
static int pt_started = 0;

static int32_t pt_resolution_ms = 1;
static PtCallback *pt_callback = NULL;
static void *pt_userdata = NULL;

static int64_t pt_now_ms(void)
{
    struct timespec ts;
#if defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    return (int64_t)ts.tv_sec * 1000 + (int64_t)(ts.tv_nsec / 1000000);
}

static void *pt_thread_main(void *arg)
{
    (void)arg;
    int64_t next = pt_now_ms();

    while (1) {
        pthread_mutex_lock(&pt_mutex);
        int running = pt_running;
        int32_t res = pt_resolution_ms;
        PtCallback *cb = pt_callback;
        void *ud = pt_userdata;
        pthread_mutex_unlock(&pt_mutex);

        if (!running) {
            break;
        }

        if (cb) {
            cb((int32_t)pt_now_ms(), ud);
        }

        if (res < 1) {
            res = 1;
        }

        next += res;
        int64_t now = pt_now_ms();
        int64_t sleep_ms = next - now;
        if (sleep_ms > 0) {
            usleep((useconds_t)(sleep_ms * 1000));
        } else {
            /* We're behind; reset schedule to avoid busy loop. */
            next = now;
            sched_yield();
        }
    }

    return NULL;
}

int Pt_Start(int resolution, PtCallback *callback, void *userData)
{
    pthread_mutex_lock(&pt_mutex);
    if (pt_started) {
        pthread_mutex_unlock(&pt_mutex);
        return ptAlreadyStarted;
    }

    pt_resolution_ms = (resolution > 0) ? resolution : 1;
    pt_callback = callback;
    pt_userdata = userData;
    pt_running = 1;
    pt_started = 1;
    pthread_mutex_unlock(&pt_mutex);

    if (pthread_create(&pt_thread, NULL, pt_thread_main, NULL) != 0) {
        pthread_mutex_lock(&pt_mutex);
        pt_running = 0;
        pt_started = 0;
        pthread_mutex_unlock(&pt_mutex);
        return ptHostError;
    }

    return ptNoError;
}

int Pt_Stop(void)
{
    pthread_mutex_lock(&pt_mutex);
    if (!pt_started) {
        pthread_mutex_unlock(&pt_mutex);
        return ptNoError;
    }
    pt_running = 0;
    pthread_mutex_unlock(&pt_mutex);

    pthread_join(pt_thread, NULL);

    pthread_mutex_lock(&pt_mutex);
    pt_started = 0;
    pt_callback = NULL;
    pt_userdata = NULL;
    pthread_mutex_unlock(&pt_mutex);

    return ptNoError;
}

int Pt_Started(void)
{
    pthread_mutex_lock(&pt_mutex);
    int started = pt_started;
    pthread_mutex_unlock(&pt_mutex);
    return started;
}

int32_t Pt_Time(void)
{
    return (int32_t)pt_now_ms();
}

void Pt_Sleep(int32_t duration)
{
    if (duration <= 0) {
        return;
    }
    usleep((useconds_t)duration * 1000);
}
