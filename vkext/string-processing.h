// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// Max preallocated size
#define STRING_PROCESS_BUF_SIZE 1000000

extern __thread int sp_errno;

/**
 * Simple memory management. Memory [pre]allocated from big static memory pool.
 * All memory allocating and deallocating - just pool pointer shifting.
 */

// Sets pool pointer to the begin. "frees" all allocated memory.
// Must be called before allocating functions.
void sp_init ();

// Preallocates 'len + 1' bytes. In fact, doesn't allocates anything,
// just checks whether there is enough available memory.
char *sp_str_pre_alloc (int len);
// Allocates 'len' bytes.
char *sp_str_alloc (int len);

// Returns sorted s string.
char *sp_sort (const char *s);

// Returns upper/lower case string for s in cp1251.
char *sp_to_upper (const char *s);
char *sp_to_lower (const char *s);

/**
 * Simplifications: look to source code to see full list of replacements.
 */

// Returns simplified s.
// Deletes all except digits, latin and russian letters in cp1251, lowercase letters.
char *sp_simplify (const char *s);

// Returns ultra-simplified s.
// Recognizes unicode characters encoded in cp1251 and html-entities. Remove diacritics
// from unicode characters, delete all except digits, latin and russian letters, lowercase
// letters. Unifies similar russian and english characters (i.e. ('n'|'п') --> 'п')
char *sp_full_simplify (const char *s);

// Converts all unicode characters encoded in cp1251 and html-entities into real cp1251,
// removing diacritics if possible. If converting is impossible - removes such characters.
char *sp_deunicode (const char *s);

char *sp_remove_repeats (const char *s);
char *sp_to_cyrillic (const char *s);
char *sp_words_only (const char *s);
