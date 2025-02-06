typedef void thread_t;
thread_t *thread_create(int (*thread_rout)(void *param), void *param);
void thread_kill(thread_t *handle);
int thread_wait(thread_t *arg);

typedef void event_t;
event_t *thread_create_event();
void thread_set_event(event_t *event);
void thread_reset_event(event_t *_event);
int thread_wait_event(event_t *event, int timeout);
void thread_destroy_event(event_t *_event);

typedef void mutex_t;
mutex_t *thread_create_mutex(void);
void thread_close_mutex(mutex_t *arg);
int thread_test_mutex(mutex_t *arg);
int thread_wait_mutex(mutex_t *arg);
int thread_release_mutex(mutex_t *mutex);

void thread_sleep(int t);
