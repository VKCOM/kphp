#ifndef __KDB_RESOLVER_H__
#define __KDB_RESOLVER_H__

#include <netdb.h>

extern int kdb_hosts_loaded;
int kdb_load_hosts (void);

struct hostent *kdb_gethostbyname (const char *name);
const char *kdb_gethostname();

#endif
