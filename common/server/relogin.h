#pragma once

const char *engine_username();
const char *engine_groupname();

void do_relogin();

// for legacy option parsers
extern const char *username, *groupname;
