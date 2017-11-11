#pragma once

#include "runtime/kphp_core.h"

string f$uniqid (const string &prefix = string(), bool more_entropy = false);


OrFalse <string> f$iconv (const string &input_encoding, const string &output_encoding, const string &input_str);


void f$sleep (const int &seconds);

void f$usleep (const int &micro_seconds);


const int IMAGETYPE_UNKNOWN = 0;
const int IMAGETYPE_GIF = 1;
const int IMAGETYPE_JPEG = 2;
const int IMAGETYPE_PNG = 3;
const int IMAGETYPE_SWF = 4;
const int IMAGETYPE_PSD = 5;
const int IMAGETYPE_BMP = 6;
const int IMAGETYPE_TIFF_II = 7;
const int IMAGETYPE_TIFF_MM = 8;
const int IMAGETYPE_JPC = 9;
const int IMAGETYPE_JPEG2000 = 9;
const int IMAGETYPE_JP2 = 10;

var f$getimagesize (const string &name);


int f$posix_getpid (void);
int f$posix_getuid (void);
OrFalse <array <var> > f$posix_getpwuid (int uid);


string f$serialize (const var &v);

var f$unserialize (const string &v);
var unserialize_raw (const char *v, int v_len);

const int JSON_UNESCAPED_UNICODE = 1;
const int JSON_FORCE_OBJECT = 16;

string f$json_encode (const var &v, int options = 0, bool simple_encode = false);

string f$vk_json_encode_safe (const var &v, bool simple_encode = true);

var f$json_decode (const string &v, bool assoc = false);

string f$print_r (const var &v, bool buffered = false);

void f$var_dump (const var &v);

string f$var_export (const var &v, bool buffered = false);


/** For local usage only **/
int f$system (const string &query);
