@ok
<?php

var_dump(ctype_alnum("abc123"));
var_dump(ctype_alnum("123"));
var_dump(ctype_alnum("abc"));
var_dump(ctype_alnum("abc 123"));
var_dump(ctype_alnum("abc!"));
var_dump(ctype_alnum(""));

var_dump(ctype_alpha("abcDEF"));
var_dump(ctype_alpha("abc"));
var_dump(ctype_alpha("ABC"));
var_dump(ctype_alpha("abc123"));
var_dump(ctype_alpha("abc!"));
var_dump(ctype_alpha(""));

var_dump(ctype_digit("123456"));
var_dump(ctype_digit("0"));
var_dump(ctype_digit("123abc"));
var_dump(ctype_digit("12.34"));
var_dump(ctype_digit(""));
var_dump(ctype_digit(" "));

var_dump(ctype_lower("abcdef"));
var_dump(ctype_lower("abc"));
var_dump(ctype_lower("abc123"));
var_dump(ctype_lower("abcDef"));
var_dump(ctype_lower("ABC"));
var_dump(ctype_lower(""));

var_dump(ctype_upper("ABCDEF"));
var_dump(ctype_upper("ABC"));
var_dump(ctype_upper("ABC123"));
var_dump(ctype_upper("abc"));
var_dump(ctype_upper("AbC"));
var_dump(ctype_upper(""));

var_dump(ctype_space(" "));
var_dump(ctype_space("\t"));
var_dump(ctype_space("\n"));
var_dump(ctype_space(" \n\t"));
var_dump(ctype_space("abc"));
var_dump(ctype_space(""));

var_dump(ctype_xdigit("0123456789abcdef"));
var_dump(ctype_xdigit("ABCDEF"));
var_dump(ctype_xdigit("123G"));
var_dump(ctype_xdigit("xyz"));
var_dump(ctype_xdigit(""));
var_dump(ctype_xdigit("0FfA"));

var_dump(ctype_cntrl("\n"));
var_dump(ctype_cntrl("\r\t"));
var_dump(ctype_cntrl("\0"));
var_dump(ctype_cntrl("abc"));
var_dump(ctype_cntrl(" "));
var_dump(ctype_cntrl(""));

var_dump(ctype_graph("abc123!@#"));
var_dump(ctype_graph("!@#"));
var_dump(ctype_graph("abc\n"));
var_dump(ctype_graph(" "));
var_dump(ctype_graph(""));
var_dump(ctype_graph("abc"));

var_dump(ctype_print("abc123!@# "));
var_dump(ctype_print(" "));
var_dump(ctype_print("abc\n"));
var_dump(ctype_print("\t"));
var_dump(ctype_print(""));
var_dump(ctype_print("abc!"));

var_dump(ctype_punct("!@#$%^&*()"));
var_dump(ctype_punct(".,;:!?"));
var_dump(ctype_punct("abc!"));
var_dump(ctype_punct(" "));
var_dump(ctype_punct(""));
var_dump(ctype_punct("!@#abc"));