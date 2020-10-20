#ifndef KDB_COMMON_PARALLEL_THREAD_ID_H
#define KDB_COMMON_PARALLEL_THREAD_ID_H

#include <sys/cdefs.h>

#define NR_THREADS 256

void parallel_register_thread(void);
void parallel_unregister_thread(void);
int parallel_online_threads_count(void);

int parallel_thread_id(void);

#endif // KDB_COMMON_PARALLEL_THREAD_ID_H
