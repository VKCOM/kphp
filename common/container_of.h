#ifndef KDB_COMMON_CONTAINER_OF_H
#define KDB_COMMON_CONTAINER_OF_H

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#endif // KDB_COMMON_CONTAINER_OF_H
