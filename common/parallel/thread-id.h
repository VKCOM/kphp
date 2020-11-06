#ifndef KDB_COMMON_PARALLEL_THREAD_ID_H
#define KDB_COMMON_PARALLEL_THREAD_ID_H

#include <sys/cdefs.h>

#define NR_THREADS 256

void parallel_register_thread();
void parallel_unregister_thread();
int parallel_online_threads_count();

int parallel_thread_id();

#endif // KDB_COMMON_PARALLEL_THREAD_ID_H
