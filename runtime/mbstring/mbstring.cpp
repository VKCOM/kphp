#include "mbstring.h"

extern "C" {
	#include <kphp/libmbfl/mbfl/mbfilter.h>
}

mbfl_string *convert_encoding(const char *str, const char *to, const char *from) {

	int len = strlen(str);
	enum mbfl_no_encoding from_encoding, to_encoding;
	mbfl_buffer_converter *convd = NULL;
	mbfl_string _string, result, *ret;

	/* from internal to mbfl */
	from_encoding = mbfl_name2no_encoding(from);
	to_encoding = mbfl_name2no_encoding(to);

	/* init buffer mbfl strings */
	mbfl_string_init(&_string);
	mbfl_string_init(&result);
	_string.no_encoding = from_encoding;
	_string.len = len;
	_string.val = (unsigned char*)str;

	/* converting */
	convd = mbfl_buffer_converter_new(from_encoding, to_encoding, 0);
	ret = mbfl_buffer_converter_feed_result(convd, &_string, &result);
	mbfl_buffer_converter_delete(convd);

	/* fix converting with multibyte encodings */
	if (len % 2 != 0 && ret->len % 2 == 0 && len < ret->len) {
		ret->len++;
		ret->val[ret->len-1] = 63;
	}
	
	return ret;
}

bool check_encoding(const char *value, const char *encoding) {

	/* init buffer mbfl strins */
	mbfl_string _string;
	mbfl_string_init(&_string);
	_string.val = (unsigned char*)value;
	_string.len = strlen((char*)value);

	/* from internal to mbfl */
	const mbfl_encoding *enc = mbfl_name2encoding(encoding);

	/* get all supported encodings */
	const mbfl_encoding **encs = mbfl_get_supported_encodings();
	int len = sizeof(**encs);

	/* identify encoding of input string */
	/* Warning! String can be represented in different encodings, so check needed */
	const mbfl_encoding *i_enc = mbfl_identify_encoding2(&_string, encs, len, 1);

	/* perform convering */
	const char *i_enc_str = (const char*)convert_encoding(value, i_enc->name, enc->name)->val;
	const char *enc_str = (const char*)convert_encoding(i_enc_str, enc->name, i_enc->name)->val;

	/* check equality */
	/* Warning! strcmp not working, because of different encodings */
	bool res = true;
	for (int i = 0; i < strlen(enc_str); i++)
		if (enc_str[i] != value[i]) {
			res = false;
			break;
		}

	free((void*)i_enc_str);
	free((void*)enc_str);
	return res;
}

bool f$mb_check_encoding(const string &value, const string &encoding) {
	const char *c_value = value.c_str();
	const char *c_encoding = encoding.c_str();
	return check_encoding(c_value, c_encoding);
}

string f$mb_convert_encoding(const string &str, const string &to_encoding, const string &from_encoding) {

	const char *c_string = str.c_str();
	const char *c_to_encoding = to_encoding.c_str();
	const char *c_from_encoding = from_encoding.c_str();

	/* perform convertion */
	mbfl_string *ret = convert_encoding(c_string, c_to_encoding, c_from_encoding);
	string res = string((const char*)ret->val, ret->len);

	/* check if string represents in from_encoding, magic number 63 - '?' in ASCII */
	if (!check_encoding(c_string, c_from_encoding)) res = string(strlen(c_string), (char)63);
	
	return res;
}