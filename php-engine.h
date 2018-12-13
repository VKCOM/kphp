#pragma once

// TODO: move it to php-lease.c
extern conn_target_t rpc_client_ct;

int get_target_by_pid(int ip, int port, conn_target_t *ct);
void send_rpc_query(struct connection *c, int op, long long id, int *q, int qsize);
struct connection *get_target_connection(conn_target_t *S, int force_flag);
int has_pending_scripts(void);