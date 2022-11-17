#pragma once

#include "runtime/kphp_core.h"
#include "runtime/string_functions.h"
#include <regex>
#include <iostream>
#include <fstream>

/*
 * Cool functions
 */

string clearSpecSymbols(const string &str);

string clearSpaces(const string &str);

string clearEOL(const string &str);

string clearQuotes(const string &str);

string clearString(const string &str);

string trim(const string &str);

/*
 * Env file|string parsing functions
 */

bool isEnvComment(const string &env_comment);

bool isEnvVar(const string &env_var);

bool isEnvVal(const string &env_val);

string get_env_var(const string &env_entry);

string get_env_val(const string &env_entry);

array<string> f$parse_env_file(const string &filename);

array<string> f$parse_env_string(const string &env_string);

