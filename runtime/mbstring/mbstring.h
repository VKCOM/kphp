#pragma once

#include "runtime/kphp_core.h"

/**
 * Check if strings are valid for the specified encoding
 * @param value The byte stream
 * @param encoding The expected encoding
 * @return Returns true on success or false on failure
 */
bool f$mb_check_encoding(const string &value, const string &encoding);

/**
 * Convert a string from one character encoding to another
 * @param str The string to be converted
 * @param from The desired encoding of the result
 * @param to The current encoding used to interpret string
 * @return The encoded string
 */
string f$mb_convert_encoding(const string &str, const string &to, const string &from);