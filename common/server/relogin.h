// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

const char* engine_username();
const char* engine_groupname();

void do_relogin();

// for legacy option parsers
extern const char *username, *groupname;
