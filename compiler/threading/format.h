#pragma once

//TODO: rename it normally
#define dl_pstr bicycle_dl_pstr
char *bicycle_dl_pstr(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
