#pragma once

const char *engine_username(void);
const char *engine_groupname(void);

void do_relogin();

// for legacy option parsers
extern const char *username, *groupname;
