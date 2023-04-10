#include "mbstring.h"

string f$mb_convert_encoding(const string &str, const string &to, const string &from) {

	/* preparing */
	const char *c_str = str.c_str();
	const char *c_from = from.c_str();
	const char *c_to = to.c_str();
	enum mbfl_no_encoding from_encoding, to_encoding;
	mbfl_buffer_converter *convd = NULL;
	mbfl_string tmp, result, *ret;

	/* from internal to mbfl */
	from_encoding = mbfl_name2no_encoding(c_from);
	to_encoding = mbfl_name2no_encoding(c_to);

	/* init buffer mbfl strings */
	long int len = strlen(c_str);
	mbfl_string_init(&tmp);
	mbfl_string_init(&result);
	tmp.no_encoding = from_encoding;
	tmp.len = len;
	tmp.val = (unsigned char*)c_str;

	/* converting */
	convd = mbfl_buffer_converter_new(from_encoding, to_encoding, 0);
	ret = mbfl_buffer_converter_feed_result(convd, &tmp, &result);
	mbfl_buffer_converter_delete(convd);

	/* returning kphp's string */
	string res((const char*)ret->val, strlen((const char*)ret->val));
	return res;
}