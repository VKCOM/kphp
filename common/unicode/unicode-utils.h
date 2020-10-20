#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Removes diacritics from unicode symbol */
int remove_diacritics (int code);

/* Prepares unicode character for search */
int prepare_search_character (int code);

/* Convert character to upper case */
int unicode_toupper (int code);

/* Convert character to lower case */
int unicode_tolower (int code);

/* Convert character to title case */
int unicode_totitle (int code);


/* Prepares unicode 0-terminated string input for search,
   leaving only digits and letters with diacritics.
   Length of string can decrease.
   Returns length of result. */
int prepare_search_string (int *input);


/* Differs prepare_search_string behaviour, forcing
   replace of character 'from' by character 'to'. */
void add_prepare_search_exception (int from, int to);


#define MAX_NAME_SIZE 65536

extern int prep_ibuf[MAX_NAME_SIZE + 4];

extern int g_no_sort_utf;

int *prepare_str_unicode (const int *x);
const char *clean_str_unicode (const int *x);
const char *clean_str (const char *x);

#ifdef __cplusplus
}
#endif
