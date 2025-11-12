@ok callback benchmark k2_skip
<?php

var_dump (preg_match_all ('~<.*>~', 'This is <something> <something else> <something further> no more', $v)); var_dump ($v);
var_dump (preg_match_all ('~.*?~', 'This', $v)); var_dump ($v);
var_dump (preg_match_all ('~.*~', 'This', $v)); var_dump ($v);

var_dump (preg_match_all ('~<.*?>~', 'This is <something> <something else> <something further> no more', $v)); var_dump ($v);

var_dump (preg_match_all ('~(\d+|)~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~(\d+\|)~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~((\d)+\|)~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~((\d)+\|)+~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~((\d+)\|)+~', '12|34|567|', $v)); var_dump ($v);


$html = "<b>bold text</b><a href=howdy.html>click me</a>";

preg_match_all("/(<([\w]+)[^>]*>)(.*?)(<\/\\2>)/", $html, $matches, PREG_SET_ORDER);

foreach ($matches as $val) {
    echo "matched: " . $val[0] . "\n";
    echo "part 1: " . $val[1] . "\n";
    echo "part 2: " . $val[2] . "\n";
    echo "part 3: " . $val[3] . "\n";
    echo "part 4: " . $val[4] . "\n\n";
}

preg_match_all("/\(?  (\d{3})?  \)?  (?(1)  [\-\s] ) \d{3}-\d{4}/x",
                "Call 555-1212 or 1-800-555-1212", $phones);

$str = <<<FOO
a: 1
b: 2
c: 3
FOO;

preg_match_all('/(?P<name>\w+): (?P<digit>\d+)/', $str, $matches);

print_r($matches);


$str0 = <<<FOO
1a: 1
b: 2
c: 3a
FOO;

for ($i = 0; $i < 8000; $i++) {
//  foreach (array('((?P<abacaba>1)?(?P<name>[a-c]+):() (?P<digit>\d+)(?P<php_bug>a)?)', '=A=i', "/(a)?/") as $pattern) {
  foreach (array('((1)?([a-c]+):() (\d+)(a)?)', '=A=i', "/(a)?/") as $pattern) {
    foreach (array($str0, '', "a", "1abAcaba", "dad") as $str) {
      preg_match_all ($pattern, $str, $matches, PREG_PATTERN_ORDER | PREG_OFFSET_CAPTURE);
      if ($i == 0) {
        var_dump ("preg_match_all($pattern, $str, PREG_PATTERN_ORDER | PREG_OFFSET_CAPTURE)");
        var_dump ($matches);
      }

      preg_match_all ($pattern, $str, $matches, PREG_SET_ORDER | PREG_OFFSET_CAPTURE);
      if ($i == 0) {
        var_dump ("preg_match_all ($pattern, $str, PREG_SET_ORDER | PREG_OFFSET_CAPTURE)");
        var_dump ($matches);
      }

      preg_match_all ($pattern, $str, $matches, PREG_PATTERN_ORDER);
      if ($i == 0) {
        var_dump ("preg_match_all ($pattern, $str, PREG_PATTERN_ORDER)");
        var_dump ($matches);
      }

      preg_match_all ($pattern, $str, $matches, PREG_SET_ORDER);
      if ($i == 0) {
        var_dump ("preg_match_all ($pattern, $str, PREG_SET_ORDER)");
        var_dump ($matches);
      }

      preg_match     ($pattern, $str, $matches, PREG_OFFSET_CAPTURE);
      if ($i == 0) {
        var_dump ("preg_match     ($pattern, $str, PREG_OFFSET_CAPTURE)");
        var_dump ($matches);
      }

      preg_match     ($pattern, $str, $matches);
      if ($i == 0) {
        var_dump ("preg_match     ($pattern, $str)");
        var_dump ($matches);
      }
    }
  }
}


foreach (array(PREG_PATTERN_ORDER, PREG_SET_ORDER) as $flag) {
  var_dump(preg_match_all('~
    (?P<date>
    (?P<year>(\d{2})?\d\d) -
    (?P<month>(?:\d\d|[a-zA-Z]{2,3})) -
    (?P<day>[0-3]?\d))
    ~x',
    '2006-05-13 e outra data: "12-Aug-37"', $m, $flag));

  var_dump($m);
}


var_dump(preg_match_all('/((?:(?:unsigned|struct)\s+)?\w+)(?:\s*(\*+)\s+|\s+(\**))(\w+(?:\[\s*\w*\s*\])?)\s*(?:(=)[^,;]+)?((?:\s*,\s*\**\s*\w+(?:\[\s*\w*\s*\])?\s*(?:=[^,;]+)?)*)\s*;/', 'unsigned int xpto = 124; short a, b;', $m, PREG_SET_ORDER));
var_dump($m);

var_dump(preg_match_all('/(?:\([^)]+\))?(&?)([\w>.()-]+(?:\[\w+\])?)\s*,?((?:\)*\s*=)?)/', '&a, b, &c', $m, PREG_SET_ORDER));
var_dump($m);

var_dump(preg_match_all('/zend_parse_parameters(?:_ex\s*\([^,]+,[^,]+|\s*\([^,]+),\s*"([^"]*)"\s*,\s*([^{;]*)/', 'zend_parse_parameters( 0, "addd|s/", a, b, &c);', $m, PREG_SET_ORDER | PREG_OFFSET_CAPTURE));
var_dump($m);


$sampledata = "
/p2/var/php_gcov/PHP_4_4/ext/ming/ming.c: In function `zif_swfbitmap_init':
/p2/var/php_gcov/PHP_4_4/ext/ming/ming.c:323: warning: assignment from incompatible pointer type
/p2/var/php_gcov/PHP_4_4/ext/ming/ming.c: In function `zif_swftextfield_setFont':
/p2/var/php_gcov/PHP_4_4/ext/ming/ming.c:2597: warning: passing arg 2 of `SWFTextField_setFont' from incompatible pointer type
/p2/var/php_gcov/PHP_4_4/ext/oci8/oci8.c:1027: warning: `oci_ping' defined but not used
/p2/var/php_gcov/PHP_4_4/ext/posix/posix.c: In function `zif_posix_getpgid':
/p2/var/php_gcov/PHP_4_4/ext/posix/posix.c:484: warning: implicit declaration of function `getpgid'
/p2/var/php_gcov/PHP_4_4/ext/posix/posix.c: In function `zif_posix_getsid':
/p2/var/php_gcov/PHP_4_4/ext/posix/posix.c:506: warning: implicit declaration of function `getsid'
/p2/var/php_gcov/PHP_4_4/ext/session/mod_files.c: In function `ps_read_files':
/p2/var/php_gcov/PHP_4_4/ext/session/mod_files.c:302: warning: implicit declaration of function `pread'
/p2/var/php_gcov/PHP_4_4/ext/session/mod_files.c: In function `ps_write_files':
/p2/var/php_gcov/PHP_4_4/ext/session/mod_files.c:340: warning: implicit declaration of function `pwrite'
/p2/var/php_gcov/PHP_4_4/ext/sockets/sockets.c: In function `zif_socket_get_option':
/p2/var/php_gcov/PHP_4_4/ext/sockets/sockets.c:1862: warning: unused variable `timeout'
/p2/var/php_gcov/PHP_4_4/ext/sockets/sockets.c: In function `zif_socket_set_option':
/p2/var/php_gcov/PHP_4_4/ext/sockets/sockets.c:1941: warning: unused variable `timeout'
/p2/var/php_gcov/PHP_4_4/regex/regexec.c:19: warning: `nope' defined but not used
/p2/var/php_gcov/PHP_4_4/ext/standard/exec.c:50: warning: `php_make_safe_mode_command' defined but not used
/p2/var/php_gcov/PHP_4_4/ext/standard/image.c: In function `php_handle_jpc':
/p2/var/php_gcov/PHP_4_4/ext/standard/image.c:604: warning: unused variable `dummy_int'
/p2/var/php_gcov/PHP_4_4/ext/standard/parsedate.c: In function `php_gd_parse':
/p2/var/php_gcov/PHP_4_4/ext/standard/parsedate.c:1138: warning: implicit declaration of function `php_gd_lex'
/p2/var/php_gcov/PHP_4_4/ext/standard/parsedate.y: At top level:
/p2/var/php_gcov/PHP_4_4/ext/standard/parsedate.y:864: warning: return type defaults to `int'
/p2/var/php_gcov/PHP_4_4/ext/sysvmsg/sysvmsg.c: In function `zif_msg_receive':
/p2/var/php_gcov/PHP_4_4/ext/sysvmsg/sysvmsg.c:318: warning: passing arg 2 of `php_var_unserialize' from incompatible pointer type
/p2/var/php_gcov/PHP_4_4/ext/yp/yp.c: In function `zif_yp_err_string':
/p2/var/php_gcov/PHP_4_4/ext/yp/yp.c:372: warning: assignment discards qualifiers from pointer target type
Zend/zend_language_scanner.c:5944: warning: `yy_fatal_error' defined but not used
Zend/zend_language_scanner.c:2627: warning: `yy_last_accepting_state' defined but not used
Zend/zend_language_scanner.c:2628: warning: `yy_last_accepting_cpos' defined but not used
Zend/zend_language_scanner.c:2634: warning: `yy_more_flag' defined but not used
Zend/zend_language_scanner.c:2635: warning: `yy_more_len' defined but not used
Zend/zend_language_scanner.c:5483: warning: `yyunput' defined but not used
Zend/zend_language_scanner.c:5929: warning: `yy_top_state' defined but not used
conflicts: 2 shift/reduce
Zend/zend_ini_scanner.c:457: warning: `yy_last_accepting_state' defined but not used
Zend/zend_ini_scanner.c:458: warning: `yy_last_accepting_cpos' defined but not used
Zend/zend_ini_scanner.c:1361: warning: `yyunput' defined but not used
/p2/var/php_gcov/PHP_4_4/Zend/zend_alloc.c: In function `_safe_emalloc':
/p2/var/php_gcov/PHP_4_4/Zend/zend_alloc.c:237: warning: long int format, size_t arg (arg 3)
/p2/var/php_gcov/PHP_4_4/Zend/zend_alloc.c:237: warning: long int format, size_t arg (arg 4)
/p2/var/php_gcov/PHP_4_4/Zend/zend_alloc.c:237: warning: long int format, size_t arg (arg 5)
/p2/var/php_gcov/PHP_4_4/Zend/zend_ini.c:338: warning: `zend_ini_displayer_cb' defined but not used
ext/mysql/libmysql/my_tempnam.o(.text+0x80): In function `my_tempnam':
/p2/var/php_gcov/PHP_4_4/ext/mysql/libmysql/my_tempnam.c:115: warning: the use of `tempnam' is dangerous, better use `mkstemp'
ext/mysql/libmysql/my_tempnam.o(.text+0x80): In function `my_tempnam':
/p2/var/php_gcov/PHP_4_4/ext/mysql/libmysql/my_tempnam.c:115: warning: the use of `tempnam' is dangerous, better use `mkstemp'
ext/ming/ming.o(.text+0xc115): In function `zim_swfmovie_namedAnchor':
/p2/var/php_gcov/PHP_5_2/ext/ming/ming.c:2207: undefined reference to `SWFMovie_namedAnchor'
/p2/var/php_gcov/PHP_5_2/ext/ming/ming.c:2209: undefined reference to `SWFMovie_xpto'
/p2/var/php_gcov/PHP_5_2/ext/ming/ming.c:2259: undefined reference to `SWFMovie_foo'
ext/ming/ming.o(.text+0x851): In function `zif_ming_setSWFCompression':
/p2/var/php_gcov/PHP_5_2/ext/ming/ming.c:154: undefined reference to `Ming_setSWFCompression'
";

$gcc_regex = '/^((.+)(\(\.text\+0x[[:xdigit:]]+\))?: In function [`\'](\w+)\':\s+)?'.
    '((?(1)(?(3)[^:\n]+|\2)|[^:\n]+)):(\d+): (?:(error|warning):\s+)?(.+)'.
    str_repeat('(?:\s+\5:(\d+): (?:(error|warning):\s+)?(.+))?', 99). // capture up to 100 errors
    '/m';


var_dump(preg_match_all($gcc_regex, $sampledata, $m, PREG_SET_ORDER));
print_r($m);


var_dump(preg_match_all('|(\w+)://([^\s"<]*[\w+#?/&=])|', "This is a text string", $matches, PREG_SET_ORDER));
var_dump($matches);

/**
 * @return mixed
 */
function func1(){
        $string = 'what the word and the other word the';
        preg_match_all('/(?P<word>the)/', $string, $matches);
        return $matches['word'];
}
$words = func1();
var_dump($words);


$pattern =
"/\s([\w_\.\/]+)(?:=([\'\"]?(?:[\w\d\s\?=\(\)\.,'_#\/\\:;&-]|(?:\\\\\"|\\\')?)+[\'\"]?))?/";
$context = "<simpletag an_attribute=\"simpleValueInside\">";

$match = array();

if ($result = preg_match_all($pattern, $context, $match))
{
var_dump($result);
var_dump($match);
}


var_dump(preg_match_all('/\d+/', '123 456 789 012', $match, 0));
var_dump($match);
