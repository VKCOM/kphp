#pragma once

#include "runtime/kphp_core.h"

extern "C" {
	// FIXME
	#include "/../../build/_deps/libmbfl-src/include/kphp/libmbfl/mbfl/mbfilter.h"
	// #include <kphp/libmbfl/mbfl/mbfilter.h>
}

/**
 * Convert a string from one character encoding to another
 * @param str The string to be converted
 * @param from The desired encoding of the result
 * @param to The current encoding used to interpret string
 * @return The encoded string
 * TODO!: mb_check_encoding(str, from) inside
 * TODO!: own constants for encodings
 * TODO: issue for timelib
 */
string f$mb_convert_encoding(const string &str, const string &to, const string &from);