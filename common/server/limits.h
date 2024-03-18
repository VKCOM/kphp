// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

int file_rlimit_init();
int raise_file_rlimit(int maxfiles);
int raise_proc_rlimit(int maxprocesses);
int raise_stack_rlimit(int maxstack);
int set_core_dump_rlimit(long long size_limit);
int adjust_oom_score(int oom_score_adj);
int get_pipe_max_limit();

#define MAX_CONNECTIONS 65536
extern int maxconn;
void set_maxconn(const char *arg);
