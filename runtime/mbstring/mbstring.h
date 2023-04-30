#pragma once

#include "runtime/kphp_core.h"
#include "common/type_traits/function_traits.h"
#include "common/vector-product.h"

#include "runtime/kphp_core.h"
#include "runtime/math_functions.h"
#include "runtime/string_functions.h"

bool mb_UTF8_check(const char *s);

#ifdef MBFL

/**
 * Check if strings are valid for the specified encoding
 * Checks if the specified byte stream is valid for the specified encoding. If value is of type array, all keys and values are validated recursively.
 * It is useful to prevent so-called "Invalid Encoding Attack".
 * @param array|string value The byte stream
 * @param ?string encoding (default = null) The expected encoding
 * @return bool Returns true on success or false on failure
 */
bool f$mb_check_encoding(const mixed &value, const Optional<string> &encoding);

/**
 * Returns a string containing the character specified by the Unicode code point value, encoded in the specified encoding 
 * @param int codepoint A Unicode codepoint value, e.g. 128024 for U+1F418 ELEPHANT
 * @param ?string encoding (default = null) The encoding parameter is the character encoding. If it is omitted or null,
 * the internal character encoding value will be used.
 * @return string|false A string containing the requested character, if it can be represented in the specified encoding or false on failure.
 */
Optional<string> f$mb_chr(const int64_t codepoint, const Optional<string> &encoding);

/**
 * Perform case folding on a string
 * @param string str The string being converted
 * @param int mode The mode of the conversion. It can be one of MB_CASE_UPPER, MB_CASE_LOWER, MB_CASE_TITLE, MB_CASE_FOLD,
 * MB_CASE_UPPER_SIMPLE, MB_CASE_LOWER_SIMPLE, MB_CASE_TITLE_SIMPLE, MB_CASE_FOLD_SIMPLE
 * @param ?string encoding (default = null) The encoding parameter is the character encoding. If it is omitted or null,
 * the internal character encoding value will be used.
 * @return string A case folded version of string converted in the way specified by mode
 */
string f$mb_convert_case(const string &str, const int64_t mode, const Optional<string> &encoding);

/**
 * Convert from one character encoding to another
 * @param array|string str The string or array to be converted
 * @param string to_encoding The desired encoding of the result
 * @param array|string|null from_encoding (default = null) The current encoding used to interpret string.
 * Multiple encodings may be specified as an array or comma separated list,
 * in which case the correct encoding will be guessed using the same algorithm as mb_detect_encoding().
 * If from_encoding is null or not specified, the mbstring.internal_encoding setting will be used if set, otherwise the default_charset setting.
 * @return array|string|false The encoded string
 */
mixed f$mb_convert_encoding(const mixed &str, const string &to_encoding, const mixed &from_encoding);

/**
 * Convert "kana" one from another ("zen-kaku", "han-kaku" and more)
 * @param string str The string being converted
 * @param string mode The conversion option (default = "KV")
 * r - Convert "zen-kaku" alphabets to "han-kaku"
 * R - Convert "han-kaku" alphabets to "zen-kaku"
 * n - Convert "zen-kaku" numbers to "han-kaku"
 * N - Convert "han-kaku" numbers to "zen-kaku"
 * a - Convert "zen-kaku" alphabets and numbers to "han-kaku"
 * A - Convert "han-kaku" alphabets and numbers to "zen-kaku" 
 * (Characters included in "a", "A" options are U+0021 - U+007E excluding U+0022, U+0027, U+005C, U+007E)
 * s - Convert "zen-kaku" space to "han-kaku" (U+3000 -> U+0020)
 * S - Convert "han-kaku" space to "zen-kaku" (U+0020 -> U+3000)
 * k - Convert "zen-kaku kata-kana" to "han-kaku kata-kana"
 * K - Convert "han-kaku kata-kana" to "zen-kaku kata-kana"
 * h - Convert "zen-kaku hira-gana" to "han-kaku kata-kana"
 * H - Convert "han-kaku kata-kana" to "zen-kaku hira-gana"
 * c - Convert "zen-kaku kata-kana" to "zen-kaku hira-gana"
 * C - Convert "zen-kaku hira-gana" to "zen-kaku kata-kana"
 * V - Collapse voiced sound notation and convert them into a character. Use with "K","H"
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used.
 * @return string The converted string
 */
string f$mb_convert_kana(const string &str, const string &mode, const Optional<string> &encoding);

/**
 * Convert character code in variable(s)
 * @param string to_encoding The encoding that the string is being converted to
 * @param array|string from_encoding is specified as an array or comma separated string, it tries to detect encoding from from-coding.
 * When from_encoding is omitted, detect_order is used.
 * @param mixed &vars References to the variable being converted. String, Array are accepted. mb_convert_variables() assumes
 * all parameters have the same encoding.
 * @return string|false The character encoding before conversion for success, or false for failure
 */
Optional<string> f$mb_convert_variables(const string &to_encoding, const mixed &from_encoding, const mixed &vars); // ???

/**
 * Decode string in MIME header field
 * @param string str The string being decoded
 * @return string The decoded string in internal character encoding
 */
string f$mb_decode_mimeheader(const string &string);

/**
 * Decode HTML numeric string reference to character
 * @param string str The string being decoded
 * @param array map An array that specifies the code area to convert
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used.
 * @return string The converted string
 */
string f$mb_decode_numericentity(const string &str, const array<int> &map, const Optional<string> &encoding);

/**
 * Detect character encoding
 * Detects the most likely character encoding for string string from an ordered list of candidates. Automatic detection of the intended character encoding
 * can never be entirely reliable; without some additional information, it is similar to decoding an encrypted string without the key. It is always preferable
 * to use an indication of character encoding stored or transmitted with the data, such as a "Content-Type" HTTP header. This function is most useful with
 * multibyte encodings, where not all sequences of bytes form a valid string. If the input string contains such a sequence, that encoding will be rejected,
 * and the next encoding checked.
 * @param string str The string being inspected
 * @param array|string|null encodings (default = null) A list of character encodings to try, in order. The list may be specified as an array of strings,
 * or a single string separated by commas. If encodings is omitted or null, the current detect_order (set with the mbstring.detect_order configuration option,
 * or mb_detect_order() function) will be used.
 * @param bool strict (default = false) Controls the behaviour when string is not valid in any of the listed encodings.
 * If strict is set to false, the closest matching encoding will be returned; if strict is set to true, false will be returned.
 * @return string|false Controls the behaviour when string is not valid in any of the listed encodings. If strict is set to false,
 * the closest matching encoding will be returned; if strict is set to true, false will be returned. The default value for strict can be set
 * with the mbstring.strict_detection configuration option.
 */
Optional<string> f$mb_detect_encoding(const string &str, const mixed &encodings, const bool strict = false);

/**
 * Set/Get character encoding detection order
 * @param array|string|null encoding (default = null) encoding is an array or comma separated list of character encoding. See supported encodings.
 * If encoding is omitted or null, it returns the current character encoding detection order as array. This setting affects
 * mb_detect_encoding() and mb_send_mail().
 * @return array|bool When setting the encoding detection order, true is returned on success or false on failure.
 * When getting the encoding detection order, an ordered array of the encodings is returned.
 */
mixed f$mb_detect_order(const mixed &encoding);

/**
 * Encode string for MIME header
 * @param string str The string being encoded. Its encoding should be same as mb_internal_encoding()
 * @param ?string charset (default = null) Specifies the name of the character set in which string is represented in.
 * The default value is determined by the current NLS setting (mbstring.language)
 * @param ?string transfer_encoding (default = null) Specifies the scheme of MIME encoding.
 * It should be either "B" (Base64) or "Q" (Quoted-Printable). Falls back to "B" if not given.
 * @param string newline (default = "\r\n") Specifies the EOL (end-of-line) marker with which mb_encode_mimeheader() performs line-folding 
 * (a » RFC term, the act of breaking a line longer than a certain length into multiple lines. The length is currently hard-coded to 74 characters).
 * Falls back to "\r\n" (CRLF) if not given.
 * @param int indent (default = 0) Indentation of the first line (number of characters in the header before string)
 * @return string A converted version of the string represented in ASCII
 */
string f$mb_encode_mimeheader(const string &str, const Optional<string> &charset, const Optional<string> &transfer_encoding, const string &newline, const int64_t indent);

/**
 * Encode character to HTML numeric string reference
 * Converts specified character codes in string string from character code to HTML numeric character reference
 * @param string str The string being encoded
 * @param array map Aarray specifies code area to convert
 * @param ?string encding (default = null) The encoding parameter is the character encoding. If it is omitted or null, the internal character encoding value will be used
 * @param bool hex (default = false) Whether the returned entity reference should be in hexadecimal notation (otherwise it is in decimal notation)
 * @return string The converted string
 */
string f$mb_encode_numericentity(const string &str, const array<int> &map, const Optional<string> &encoding, const bool hex = false);

/**
 * Get aliases of a known encoding type
 * @param string encoding The encoding type being checked, for aliases
 * @return array Returns a numerically indexed array of encoding aliases
 */
array<string> f$mb_encoding_aliases(const string &encoding);

/**
 * Regular expression match for multibyte string
 * @param string pattern The regular expression pattern
 * @param string str The string being evaluated
 * @param ?string options (default = null) The search option. See mb_regex_set_options() for explanation
 * @return bool Returns true if string matches the regular expression pattern, false if not
 */
bool f$mb_ereg_match(const string &pattern, const string &str, const Optional<string> &options);

/**
 * Perform a regular expression search and replace with multibyte support using a callback
 * Scans string for matches to pattern, then replaces the matched text with the output of callback function.
 * The behavior of this function is almost identical to mb_ereg_replace(), except for the fact that instead of replacement parameter,
 * one should specify a callback.
 * @param string pattern The regular expression pattern. Multibyte characters may be used in pattern.
 * @param callable callback A callback that will be called and passed an array of matched elements in the subject string. 
 * The callback should return the replacement string. You'll often need the callback function for a mb_ereg_replace_callback() in just one place. 
 * In this case you can use an anonymous function to declare the callback within the call to mb_ereg_replace_callback(). 
 * By doing it this way you have all information for the call in one place and do not clutter the function namespace with a callback
 * function's name not used anywhere else.
 * @param string str The string being checked
 * @param ?string options (default = null) The search option. See mb_regex_set_options() for explanation
 * @return string|false|null The resultant string on success, or false on error. If string is not valid for the current encoding, null is returned
 */
// Optional<string> f$mb_ereg_replace_callback(const string &pattern, const CallableT &callback, const string &str, const Optional<string> options); // callback

/**
 * Replace regular expression with multibyte support
 * Scans string for matches to pattern, then replaces the matched text with replacement
 * @param string pattern The regular expression pattern. Multibyte characters may be used in pattern
 * @param string replacement The replacement text
 * @param string str The string being checked
 * @param ?string options (default = null) The search option. See mb_regex_set_options() for explanation
 * @return string|false|null The resultant string on success, or false on error. If string is not valid for the current encoding, null is returned
 */
Optional<string> f$mb_ereg_replace(const string &pattern, const string &replacement, const string &str, const Optional<string> &options);

/**
 * Returns start point for next regular expression match
 * @return int mb_ereg_search_getpos() returns the point to start regular expression match for mb_ereg_search(), mb_ereg_search_pos(), mb_ereg_search_regs().
 * The position is represented by bytes from the head of string.
 */
int64_t f$mb_ereg_search_getpos(void);

/**
 * Retrieve the result from the last multibyte regular expression match
 * @return array|false An array including the sub-string of matched part by last mb_ereg_search(), mb_ereg_search_pos(), mb_ereg_search_regs().
 * If there are some matches, the first element will have the matched sub-string, the second element will have the first part grouped with brackets,
 * the third element will have the second part grouped with brackets, and so on. It returns false on error.
 */
mixed f$mb_ereg_search_getregs(void);

/**
 * Setup string and regular expression for a multibyte regular expression match
 * mb_ereg_search_init() sets string and pattern for a multibyte regular expression.
 * These values are used for mb_ereg_search(), mb_ereg_search_pos(),and mb_ereg_search_regs().
 * @param string str The search string
 * @param ?string pattern (default = null) The search pattern
 * @param ?string options (default = null) The search option. See mb_regex_set_options() for explanation
 * @return bool Returns true on success or false on failure
 */
bool f$mb_ereg_search_init(const string &str, const Optional<string> &pattern, const Optional<string> &options);

/**
 * Returns position and length of a matched part of the multibyte regular expression for a predefined multibyte string
 * The string for match is specified by mb_ereg_search_init(). If it is not specified, the previous one will be used
 * @param ?string pattern (default = null) The search pattern
 * @param ?string options (default = null) The search option. See mb_regex_set_options() for explanation
 * @return array|false An array containing two elements. The first element is the offset, in bytes, where the match begins relative to the start of
 * the search string, and the second element is the length in bytes of the match. If an error occurs, false is returned.
 */
mixed f$mb_ereg_search_pos(const Optional<string> &pattern, const Optional<string> &options);

/**
 * Returns the matched part of a multibyte regular expression
 * @param ?string pattern (default = null) The search pattern
 * @param ?string options (deafult = null) The search option. See mb_regex_set_options() for explanation
 * @return array|false mb_ereg_search_regs() executes the multibyte regular expression match, and if there are some matched part,
 * it returns an array including substring of matched part as first element, the first grouped part with brackets as second element,
 * the second grouped part as third element, and so on. It returns false on error.
 */
mixed f$mb_ereg_search_regs(const Optional<string> &pattern, const Optional<string> &options);

/**
 * Set start point of next regular expression match
 * mb_ereg_search_setpos() sets the starting point of a match for mb_ereg_search().
 * @param int offset The position to set. If it is negative, it counts from the end of the string
 * @return bool Returns true on success or false on failure
 */
bool f$mb_ereg_search_setpos(const int64_t offset);

/**
 * Multibyte regular expression match for predefined multibyte string
 * @param ?string pattern (default = null) The search pattern
 * @param ?string options (default = null) The search option. See mb_regex_set_options() for explanation
 * @return bool mb_ereg_search() returns true if the multibyte string matches with the regular expression, or false otherwise. The string for matching
 * is set by mb_ereg_search_init(). If pattern is not specified, the previous one is used.
 */
bool f$mb_ereg_search(const Optional<string> &pattern, const Optional<string> &options);

/**
 * Regular expression match with multibyte support
 * @param string pattern The search pattern
 * @param string str The search string
 * @param array matches (default = null) If matches are found for parenthesized substrings of pattern and the function is called with the
 * third argument matches, the matches will be stored in the elements of the array matches. If no matches are found, matches is set to an empty array.
 * matches[1] will contain the substring which starts at the first left parenthesis; $matches[2] will contain the substring starting at the second,
 * and so on. $matches[0] will contain a copy of the complete string matched.
 * @return bool Returns whether pattern matches string
 */
bool f$mb_ereg(const string &pattern, const string &str, const array<string> &matches);

/**
 * Replace regular expression with multibyte support ignoring case
 * Scans string for matches to pattern, then replaces the matched text with replacement
 * @param string pattern The regular expression pattern. Multibyte characters may be used. The case will be ignored
 * @param string replacement The replacement text
 * @param string str The searched string
 * @param ?string options (default = null) The search option. See mb_regex_set_options() for explanation
 * @return string|false|null The resultant string or false on error. If string is not valid for the current encoding, null is returned
 */
Optional<string> f$mb_eregi_replace(const string &pattern, const string &replacement, const string &str, const Optional<string> &options);

/**
 * Regular expression match ignoring case with multibyte support
 * @param string pattern The regular expression pattern
 * @param string str The string being searched
 * @param array matches (default = null) If matches are found for parenthesized substrings of pattern and the function is called with the third argument matches,
 * the matches will be stored in the elements of the array matches. If no matches are found, matches is set to an empty array.
 * matches[1] will contain the substring which starts at the first left parenthesis; $matches[2] will contain the substring starting at the second,
 * and so on. $matches[0] will contain a copy of the complete string matched.
 * @return bool Returns whether pattern matches string
 */
bool f$mb_eregi(const string &pattern, const string &str, const array<string> &matches);

/**
 * Get internal settings of mbstring
 * @param string type (default = "all") If type is not specified or is specified as "all", "internal_encoding", "http_input", "http_output",
 * "http_output_conv_mimetypes", "mail_charset", "mail_header_encoding", "mail_body_encoding", "illegal_chars", "encoding_translation", "language",
 * "detect_order", "substitute_character" and "strict_detection" will be returned.
 * If type is specified as "internal_encoding", "http_input", "http_output", "http_output_conv_mimetypes", "mail_charset", "mail_header_encoding",
 * "mail_body_encoding", "illegal_chars", "encoding_translation", "language", "detect_order", "substitute_character" or "strict_detection"
 * the specified setting parameter will be returned.
 * @return array|string|int|false An array of type information if type is not specified, otherwise a specific type, or false on failure
 */
mixed f$mb_get_info(const string &type);

/**
 * Detect HTTP input character encoding
 * @param ?string type (default = null) Input string specifies the input type. "G" for GET, "P" for POST, "C" for COOKIE, "S" for string,
 * "L" for list, and "I" for the whole list (will return array). If type is omitted, it returns the last input type processed.
 * @return array|string|false The character encoding name, as per the type, or an array of character encoding names, if type is "I".
 * If mb_http_input() does not process specified HTTP input, it returns false.
 */
mixed f$mb_http_input(const Optional<string> &type);

/**
 * Set/Get the HTTP output character encoding. Output after this function is called will be converted from the set internal encoding to encoding
 * @param ?string encoding (default = null) If encoding is set, mb_http_output() sets the HTTP output character encoding to encoding.
 * If encoding is omitted, mb_http_output() returns the current HTTP output character encoding.
 * @return string|bool If encoding is omitted, mb_http_output() returns the current HTTP output character encoding. Otherwise,
 * Returns true on success or false on failure.
 */
mixed f$mb_http_output(const Optional<string> &encoding);

/**
 * Set/Get internal character encoding
 * @param ?string encoding (default = null) encoding is the character encoding name used for the HTTP input character encoding conversion,
 * HTTP output character encoding conversion, and the default character encoding for string functions defined by the mbstring module.
 * You should notice that the internal encoding is totally different from the one for multibyte regex.
 * @return string|bool If encoding is set, then Returns true on success or false on failure.
 * In this case, the character encoding for multibyte regex is NOT changed.
 * If encoding is omitted, then the current character encoding name is returned.
 */
mixed f$mb_internal_encoding(const Optional<string> &encoding);

/**
 * Set/Get the current language
 * @param ?string language (default = null) Used for encoding e-mail messages. The valid languages are listed in the following table.
 * mb_send_mail() uses this setting to encode e-mail.
 * +---------------------------+-------------+------------------+-----------+
 * | Language                  | Charset     | Encoding         | Alias     |
 * +---------------------------+-------------+------------------+-----------+
 * | German/de                 | ISO-8859-15 | Quoted-Printable | Deutsch   |
 * | English/en                | ISO-8859-1  | Quoted-Printable |           |
 * | Armenian/hy               | ArmSCII-8   | Quoted-Printable |           |
 * | Japanese/ja               | ISO-2022-JP | BASE64           |           |
 * | Korean/ko                 | ISO-2022-KR | BASE64           |           |
 * | neutral                   | UTF-8       | BASE64           |           |
 * | Russian/ru                | KOI8-R      | Quoted-Printable |           |
 * | Turkish/tr                | ISO-8859-9  | Quoted-Printable |           |
 * | Ukrainian/ua              | KOI8-U      | Quoted-Printable |           |
 * | uni                       | UTF-8       | BASE64           | universal |
 * | Simplified Chinese/zh-cn  | HZ          | BASE64           |           |
 * | Traditional Chinese/zh-tw | BIG-5       | BASE64           |           |   
 * +---------------------------+-------------+------------------+-----------+
 * @return string|bool If language is set and language is valid, it returns true. Otherwise, it returns false. When language is omitted or null,
 * it returns the language name as a string
 */
mixed f$mb_language(const Optional<string> &language);

/**
 * Returns an array of all supported encodings
 * @return array Returns a numerically indexed array
 */
array<string> f$mb_list_encodings(void);

/**
 * Returns the Unicode code point value of the given character. This function complements mb_chr().
 * @param string str A string
 * @param string? encoding (default = null) The encoding parameter is the character encoding. If it is omitted or null,
 * the internal character encoding value will be used.
 * @return int|false The Unicode code point for the first character of string or false on failure.
 */
Optional<int> f$mb_ord(const string &str, const Optional<string> &encoding);

/**
 * mb_output_handler() is ob_start() callback function. mb_output_handler() converts characters in the output buffer from internal
 * character encoding to HTTP output character encoding.
 * @param string str The contents of the output buffer
 * @param int status The status of the output buffer
 * @return string The converted string
 */
string f$mb_output_handler(const string &str, const int64_t status);

/**
 * Parses GET/POST/COOKIE data and sets global variables. Since PHP does not provide raw POST/COOKIE data, it can only be used for GET data for now.
 * It parses URL encoded data, detects encoding, converts coding to internal encoding and set values to the result array or global variables.
 * @param string str The URL encoded data
 * @param array result An array containing decoded and character encoded converted values
 * @return bool Returns true on success or false on failure
 */
bool f$mb_parse_str(const string &str, const array<string> &result); // result = map<string, string>

/**
 * Get a MIME charset string for a specific encoding.
 * @param string encoding The encoding being checked
 * @return string|false The MIME charset string for character encoding encoding, or false if no charset is preferred for the given encoding
 */
Optional<string> f$mb_preferred_mime_name(const string &encoding);

/**
 * Set/Get character encoding for a multibyte regex
 * @param ?string encoding (default = null) The encoding parameter is the character encoding. If it is omitted or null,
 * the internal character encoding value will be used
 * @return string|bool If encoding is set, then Returns true on success or false on failure. In this case, the internal character encoding is NOT changed.
 * If encoding is omitted, then the current character encoding name for a multibyte regex is returned
 */
mixed f$mb_regex_encoding(const Optional<string> &encoding);

/**
 * Sets the default options described by options for multibyte regex functions
 * @param ?string options (default = null) The options to set. This is a string where each character is an option.
 * To set a mode, the mode character must be the last one set, however there can only be set one mode but multiple options
 * 
 * Regex options:
 * +--------+----------------------------------+
 * | Option | Meaning                          |
 * +--------+----------------------------------+ 
 * | i      | Ambiguity match on               |
 * | x      | Enables extended pattern form    |
 * | m      | '.' matches with newlines        |
 * | s      | '^' -> '\A', '$' -> '\Z'         |
 * | p      | Same as both the m and s options |
 * | l      | Finds longest matches            |
 * | n      | Ignores empty matches            |
 * | e      | eval() resulting code            |
 * +--------+----------------------------------+
 * 
 * Regex syntax modes:
 * +------+----------------------------+
 * | Mode | Meaning                    |
 * +------+----------------------------+
 * | j    | Java (Sun java.util.regex) |
 * | u    | GNU regex                  | 
 * | g    | grep                       |
 * | c    | Emacs                      |
 * | r    | Ruby                       |
 * | z    | Perl                       |
 * | b    | POSIX Basic regex          |
 * | d    | POSIX Extended regex       |
 * +------+----------------------------+
 *
 * @return string The previous options. If options is omitted or null, it returns the string that describes the current options
 */
string f$mb_regex_set_options(const Optional<string> &options);

/**
 * This function is currently not documented; only its argument list is available.
 * @param string str
 * @param ?string encoding (default = null)
 * @return string
 */
string f$mb_scrub(const string &str, const Optional<string> &encoding);

/**
 * Sends email. Headers and messages are converted and encoded according to the mb_language() setting.
 * It's a wrapper function for mail(), so see also mail() for detail
 * @param string to The mail addresses being sent to. Multiple recipients may be specified by putting a comma between each address in to.
 * This parameter is not automatically encoded
 * @param string subject The subject of the mail
 * @param string message The message of the mail
 * @param array|string additional_headers (default = []) String or array to be inserted at the end of the email header.
 * This is typically used to add extra headers (From, Cc, and Bcc). Multiple extra headers should be separated with a CRLF (\r\n).
 * Validate parameter not to be injected unwanted headers by attackers. If an array is passed, its keys are the header names and its
 * values are the respective header values
 * Note:
 * If messages are not received, try using a LF (\n) only. Some Unix mail transfer agents (most notably » qmail) replace LF by CRLF automatically
 * (which leads to doubling CR if CRLF is used). This should be a last resort, as it does not comply with » RFC 2822.
 * @param ?string additional_params (default = null) additional_params is a MTA command line parameter. It is useful when setting the correct Return-Path header
 * when using sendmail. This parameter is escaped by escapeshellcmd() internally to prevent command execution. escapeshellcmd() prevents command execution,
 * but allows to add additional parameters. For security reason, this parameter should be validated. Since escapeshellcmd() is applied automatically,
 * some characters that are allowed as email addresses by internet RFCs cannot be used. Programs that are required to use these characters mail() cannot be used.
 * The user that the webserver runs as should be added as a trusted user to the sendmail configuration to prevent a 'X-Warning' header from being added to
 * the message when the envelope sender (-f) is set using this method. For sendmail users, this file is /etc/mail/trusted-users
 * @return bool Returns true on success or false on failure
 */
bool f$mb_send_mail(const string &to, const string &subject, const string &message, const mixed &additional_headers, const Optional<string> &additional_params);

/**
 * Split a multibyte string using regular expression pattern and returns the result as an array
 * @param string pattern The regular expression pattern
 * @param string str The string being split
 * @param int limit (default = -1) If optional parameter limit is specified, it will be split in limit elements as maximum
 * @return array|false The result as an array, or false on failure
 */
mixed f$mb_split(const string &pattern, const string &str, const int64_t limit = -1);

/**
 * This function will return an array of strings, it is a version of str_split() with support for encodings of variable character size as well
 * as fixed-size encodings of 1,2 or 4 byte characters. If the length parameter is specified, the string is broken down into chunks of the specified
 * length in characters (not bytes). The encoding parameter can be optionally specified and it is good practice to do so
 * @param string str The string to split into characters or chunks
 * @param int length (default = 1) If specified, each element of the returned array will be composed of multiple characters instead of a single character
 * @param ?string encoding (default = null) The encoding parameter is the character encoding. If it is omitted or null, the internal character encoding value
 * will be used. A string specifying one of the supported encodings
 * @return array mb_str_split() returns an array of strings
 */
array<string> f$mb_str_split(const string &str, const int64_t length, const Optional<string> &encoding);

/**
 * mb_strcut() extracts a substring from a string similarly to mb_substr(), but operates on bytes instead of characters.
 * If the cut position happens to be between two bytes of a multi-byte character, the cut is performed starting from the first byte of that character.
 * This is also the difference to the substr() function, which would simply cut the string between the bytes and thus result in a malformed byte sequence
 * @param string str The string being cut
 * @param int start If start is non-negative, the returned string will start at the start'th byte position in string, counting from zero.
 * For instance, in the string 'abcdef', the byte at position 0 is 'a', the byte at position 2 is 'c', and so forth.
 * If start is negative, the returned string will start at the start'th byte counting back from the end of string.
 * However, if the magnitude of a negative start is greater than the length of the string, the returned portion will start from the beginning of string
 * @param ?int length (default = null) Length in bytes. If omitted or NULL is passed, extract all bytes to the end of the string.
 * If length is negative, the returned string will end at the length'th byte counting back from the end of string.
 * However, if the magnitude of a negative length is greater than the number of characters after the start position, an empty string will be returned
 * @param ?string encoding The encoding parameter is the character encoding. If it is omitted or null, the internal character encoding value will be used
 * @return string mb_strcut() returns the portion of string specified by the start and length parameters
 */
string f$mb_strcut(const string &str, const int64_t start, const Optional<int64_t> &length, const Optional<string> &encoding);

/**
 * Truncates string string to specified width, where halfwidth characters count as 1, and fullwidth characters count as 2.
 * See » http://www.unicode.org/reports/tr11/ for details regarding East Asian character widths
 * @param string str The string being decoded
 * @param int start The start position offset. Number of characters from the beginning of string (first character is 0),
 * or if start is negative, number of characters from the end of the string
 * @param int width The width of the desired trim. Negative widths count from the end of the string
 * @param string trim_marker (default = "") A string that is added to the end of string when string is truncated
 * @param ?string encoding (default = null) The encoding parameter is the character encoding. If it is omitted or null,
 * the internal character encoding value will be used
 * @return string The truncated string. If trim_marker is set, trim_marker replaces the last chars to match the width
 */
string f$mb_strimwidth(const string &str, const int64_t start, const int64_t width, const string &trim_marker, const Optional<string> &encoding);

/**
 * mb_stripos() returns the numeric position of the first occurrence of needle in the haystack string. Unlike mb_strpos(),
 * mb_stripos() is case-insensitive. If needle is not found, it returns false
 * @param string haystack The string from which to get the position of the first occurrence of needl
 * @param string needle The string to find in haystack
 * @param int offset (default = 0) The position in haystack to start searching. A negative offset counts from the end of the string
 * @param ?string encoding (default = null) Character encoding name to use. If it is omitted, internal character encoding is used
 * @return int|false Return the numeric position of the first occurrence of needle in the haystack string, or false if needle is not found
 */
Optional<int> f$mb_stripos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding);

/**
 * mb_stristr() finds the first occurrence of needle in haystack and returns the portion of haystack.
 * Unlike mb_strstr(), mb_stristr() is case-insensitive. If needle is not found, it returns false
 * @param string haystack The string from which to get the first occurrence of needle
 * @param string needle The string to find in haystack
 * @param bool before_needle (default = false) Determines which portion of haystack this function returns.
 * If set to true, it returns all of haystack from the beginning to the first occurrence of needle (excluding needle).
 * If set to false, it returns all of haystack from the first occurrence of needle to the end (including needle)
 * @param ?string encoding (default = null) Character encoding name to use. If it is omitted, internal character encoding is used
 * @return string|false Returns the portion of haystack, or false if needle is not found
 */
Optional<string> f$mb_stristr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding);

/**
 * Gets the length of a string
 * @param string str The string being checked for length
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used
 * @return int Returns the number of characters in string string having character encoding encoding. A multi-byte character is counted as 1
 */
int64_t f$mb_strlen(const string &str, const Optional<string> &encoding);

/**
 * Finds position of the first occurrence of a string in a string. Performs a multi-byte safe strpos() operation based on number of characters.
 * The first character's position is 0, the second character position is 1, and so on
 * @param string haystack The string being checked
 * @param string needle The string to find in haystack. In contrast with strpos(), numeric values are not applied as the ordinal value of a character
 * @param int offset (default = 0) The search offset. If it is not specified, 0 is used. A negative offset counts from the end of the string
 * @param ?string encoding (default = null) The encoding parameter is the character encoding. If it is omitted or null,
 * the internal character encoding value will be used
 * @return int|false Returns the numeric position of the first occurrence of needle in the haystack string. If needle is not found, it returns false
 */
Optional<int> f$mb_strpos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding);

/**
 * mb_strrchr() finds the last occurrence of needle in haystack and returns the portion of haystack. If needle is not found, it returns false
 * @param string haystack The string from which to get the last occurrence of needle
 * @param string needle The string to find in haystack
 * @param bool before_needle Determines which portion of haystack this function returns.
 * If set to true, it returns all of haystack from the beginning to the last occurrence of needle.
 * If set to false, it returns all of haystack from the last occurrence of needle to the end
 * @param ?string encoding (default = null) Character encoding name to use. If it is omitted, internal character encoding is used
 * @return string|false Returns the portion of haystack. or false if needle is not found
 */
Optional<string> f$mb_strrchr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding);

/**
 * mb_strrichr() finds the last occurrence of needle in haystack and returns the portion of haystack. Unlike mb_strrchr(), mb_strrichr() is case-insensitive.
 * If needle is not found, it returns false
 * @param string haystack The string from which to get the last occurrence of needle
 * @param string needle The string to find in haystack
 * @param bool before_needle Determines which portion of haystack this function returns.
 * If set to true, it returns all of haystack from the beginning to the last occurrence of needle.
 * If set to false, it returns all of haystack from the last occurrence of needle to the end
 * @param ?string encoding (default = null)
 * @return string|false Character encoding name to use. If it is omitted, internal character encoding is used
 */
Optional<string> f$mb_strrichr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding);

/**
 * mb_strripos() performs multi-byte safe strripos() operation based on number of characters. needle position is counted from the beginning of haystack.
 * First character's position is 0. Second character position is 1. Unlike mb_strrpos(), mb_strripos() is case-insensitive
 * @param string haystack The string from which to get the position of the last occurrence of needle
 * @param string needle The string to find in haystack
 * @param int offset The position in haystack to start searching
 * @param ?string encoding (default = null) Character encoding name to use. If it is omitted, internal character encoding is used
 * @return int|false Return the numeric position of the last occurrence of needle in the haystack string, or false if needle is not found
 */
Optional<int> f$mb_strripos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding);

/**
 * Performs a multibyte safe strrpos() operation based on the number of characters. needle position is counted from the beginning of haystack.
 * First character's position is 0. Second character position is 1
 * @param string haystack The string being checked, for the last occurrence of needle
 * @param string needle The string to find in haystack
 * @param int offset (default = 0) May be specified to begin searching an arbitrary number of characters into the string. Negative values will stop searching at an arbitrary point prior to the end of the string
 * @param ?string encoding The encoding parameter is the character encoding. If it is omitted or null, the internal character encoding value will be used
 * @return int|false Returns the numeric position of the last occurrence of needle in the haystack string. If needle is not found, it returns false
 */
Optional<int> f$mb_strrpos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding);

/**
 * mb_strstr() finds the first occurrence of needle in haystack and returns the portion of haystack. If needle is not found, it returns false
 * @param string haystack The string from which to get the first occurrence of needle
 * @param string needle The string to find in haystack
 * @param bool before_needle Determines which portion of haystack this function returns.
 * If set to true, it returns all of haystack from the beginning to the first occurrence of needle (excluding needle).
 * If set to false, it returns all of haystack from the first occurrence of needle to the end (including needle)
 * @param ?string encoding (default = null) Character encoding name to use. If it is omitted, internal character encoding is used
 * @return string|false Returns the portion of haystack, or false if needle is not found
 */
Optional<string> f$mb_strstr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding);

/**
 * Returns string with all alphabetic characters converted to lowercase
 * @param string str The string being lowercased
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used
 * @return string string with all alphabetic characters converted to lowercase
 */
string f$mb_strtolower(const string &str, const Optional<string> &encoding);

/**
 * Returns string with all alphabetic characters converted to uppercase.
 * @param string str The string being uppercased
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used
 * @return string string with all alphabetic characters converted to uppercase
 */
string f$mb_strtoupper(const string &str, const Optional<string> &encoding);

/**
 * Returns the width of string string, where halfwidth characters count as 1, and fullwidth characters count as 2.
 * See » http://www.unicode.org/reports/tr11/ for details regarding East Asian character widths. The fullwidth characters are:
 * U+1100-U+115F, U+11A3-U+11A7, U+11FA-U+11FF, U+2329-U+232A, U+2E80-U+2E99, U+2E9B-U+2EF3, U+2F00-U+2FD5, U+2FF0-U+2FFB, U+3000-U+303E, U+3041-U+3096,
 * U+3099-U+30FF, U+3105-U+312D, U+3131-U+318E, U+3190-U+31BA, U+31C0-U+31E3, U+31F0-U+321E, U+3220-U+3247, U+3250-U+32FE, U+3300-U+4DBF, U+4E00-U+A48C,
 * U+A490-U+A4C6, U+A960-U+A97C, U+AC00-U+D7A3, U+D7B0-U+D7C6, U+D7CB-U+D7FB, U+F900-U+FAFF, U+FE10-U+FE19, U+FE30-U+FE52, U+FE54-U+FE66, U+FE68-U+FE6B,
 * U+FF01-U+FF60, U+FFE0-U+FFE6, U+1B000-U+1B001, U+1F200-U+1F202, U+1F210-U+1F23A, U+1F240-U+1F248, U+1F250-U+1F251, U+20000-U+2FFFD, U+30000-U+3FFFD.
 * All other characters are halfwidth characters
 * @param string str The string being decoded
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used
 * @return int The width of string string
 */
int64_t f$mb_strwidth(const string &str, const Optional<string> &encoding);

/**
 * Specifies a substitution character when input character encoding is invalid or character code does not exist in output character encoding.
 * Invalid characters may be substituted "none" (no output), string or int value (Unicode character code value).
 * This setting affects mb_convert_encoding(), mb_convert_variables(), mb_output_handler(), and mb_send_mail()
 * @param string|int|null substitute_character (default = null) Specify the Unicode value as an int, or as one of the following strings:
 * "none": no output
 * "long": Output character code value (Example: U+3000, JIS+7E7E)
 * "entity": Output character entity (Example: &#x200;)
 * @return string|int|bool If substitute_character is set, it returns true for success, otherwise returns false.
 * If substitute_character is not set, it returns the current setting
 */
mixed f$mb_substitute_character(const mixed &substitute_character);

/**
 * Counts the number of times the needle substring occurs in the haystack string
 * @param string haystack The string being checked
 * @param string needle The string being found
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used
 * @return int The number of times the needle substring occurs in the haystack string
 */
int64_t f$mb_substr_count(const string &haystack, const string &needle, const Optional<string> &encoding);

/**
 * Performs a multi-byte safe substr() operation based on number of characters. Position is counted from the beginning of string.
 * First character's position is 0. Second character position is 1, and so on
 * @param string str The string to extract the substring from
 * @param int start If start is non-negative, the returned string will start at the start'th position in string, counting from zero.
 * For instance, in the string 'abcdef', the character at position 0 is 'a', the character at position 2 is 'c', and so forth.
 * If start is negative, the returned string will start at the start'th character from the end of string
 * @param ?int length (default = null) Maximum number of characters to use from string.
 * If omitted or NULL is passed, extract all characters to the end of the string
 * @param ?string encoding (default = null) The encoding parameter is the character encoding.
 * If it is omitted or null, the internal character encoding value will be used
 * @return string mb_substr() returns the portion of string specified by the start and length parameters
 */
string f$mb_substr(const string &str, const int64_t start, const Optional<int64_t> &length, const Optional<string> &encoding);

#else

#include <climits>

#include "runtime/kphp_core.h"
#include "runtime/string_functions.h"

bool f$mb_check_encoding(const string &str, const string &encoding = CP1251);

int64_t f$mb_strlen(const string &str, const string &encoding = CP1251);

string f$mb_strtolower(const string &str, const string &encoding = CP1251);

string f$mb_strtoupper(const string &str, const string &encoding = CP1251);

Optional<int64_t> f$mb_strpos(const string &haystack, const string &needle, int64_t offset = 0, const string &encoding = CP1251) noexcept;

Optional<int64_t> f$mb_stripos(const string &haystack, const string &needle, int64_t offset = 0, const string &encoding = CP1251) noexcept;

string f$mb_substr(const string &str, int64_t start, const mixed &length = std::numeric_limits<int64_t>::max(), const string &encoding = CP1251);

void f$set_detect_incorrect_encoding_names_warning(bool show);

void free_detect_incorrect_encoding_names();

#endif