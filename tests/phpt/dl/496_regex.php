@ok callback benchmark k2_skip
<?php
var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '\26\25\24\23\22\21\20\19\18\17\16\15\14\13\12\11\10\9\8\7\6\5\4\3\2\1', 'abcdefghijklmnopqrstuvwxyz'));
var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '$26$25$24$23$22$21$20$19$18$17$16$15$14$13$12$11$10$9$8$7$6$5$4$3$2$1', 'abcdefghijklmnopqrstuvwxyz'));
var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '$26$25$24$23$22$21$20$19$18$17$16$15$14$13$12$11$10$9$8$7$6$5$4$3$2$1', 'abcdefghijklmnopqrstuvwxyz'));
var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '${26}${25}${24}${23}${22}${21}${20}${19}${18}${17}${16}${15}${14}${13}${12}${11}${10}${9}${8}${7}${6}${5}${4}${3}${2}${1}', 'abcdefghijklmnopqrstuvwxyz'));

var_dump (preg_replace ('~a|~', 'b', 'c'));
var_dump (preg_replace ('~a|~', 'b', 'a'));
var_dump (preg_replace ('~a|~', 'a', 'b'));
var_dump (preg_match_all ('~<.*>~', 'This is <something> <something else> <something further> no more', $v)); var_dump ($v);
var_dump (preg_match_all ('~.*?~', 'This', $v)); var_dump ($v);
var_dump (preg_match_all ('~.*~', 'This', $v)); var_dump ($v);

var_dump (preg_match_all ('~<.*?>~', 'This is <something> <something else> <something further> no more', $v)); var_dump ($v);

var_dump (preg_match_all ('~(\d+|)~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~(\d+\|)~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~((\d)+\|)~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~((\d)+\|)+~', '12|34|567|', $v)); var_dump ($v);
var_dump (preg_match_all ('~((\d+)\|)+~', '12|34|567|', $v)); var_dump ($v);

var_dump (preg_replace ('~|q~', '{\0}', 'eq'));
var_dump (preg_replace ('~|q~', '{\0}', 'ex'));

var_dump (preg_replace ('~|q~', 'w', 'e'));
var_dump (preg_replace ('~|q~', 'w', 'q'));
/* bug in PHP
var_dump (preg_replace ('~|Ð¹~u', 'Ð¿', 'Ñ€'));
*/
var_dump (preg_replace ('~|Ð¹~u', 'Ð¿', 'Ð¹'));

var_dump (preg_split ('~|Ð¹~u', 'Ð¿'));
var_dump (preg_split ('~|Ð¹~u', 'Ð¹'));


define('RE_URL_PATTERN', '(?<![A-Za-z\$0-9À-ßà-ÿ¸¨\-\_])(https?:\/\/)?((?:[A-Za-z\$0-9À-ßà-ÿ¸¨](?:[A-Za-z\$0-9\-\_À-ßà-ÿ¸¨]*[A-Za-z\$0-9À-ßà-ÿ¸¨])?\.){1,5}[A-Za-z\$ðôóêÐÔÓÊ\-\d]{2,22}(?::\d{2,5})?)((?:\/(?:(?:\&amp;|\&#33;|,[_%]|[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.~=;:]+|\[[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\]|\([A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\))*(?:,[_%]|[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿\-\_#%?+\/\$.~=;:]*[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\_#%?+\/\$~=]|\[[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\]|\([A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\)))?)?)');

$text = 'ß ñëûøàë, ÷òî â iOS 7 ïîÿâèëèñü ëîêàëüíûå ïóø-óâåäîìëåíèÿ. Íî òóò http://blog.derand.net/2010/08/local-notifications-ios-40.html óòâåðæäàåòñÿ, ÷òî åùå â ÷åòâåðòîé.';
$text = preg_replace_callback('/'.RE_URL_PATTERN.'/', 'prcConvertHyperref', $text);

/**
 * @kphp-required
 * @param string[] $matches
 * @return string
 */
function prcConvertHyperref($matches) {
  return (string)preg_match('/\.([a-zA-ZðôóêÐÔÓÊ\-0-9]+)$/', $matches[2], $match);
}

$string = 'April 15, 2003';
$pattern = '/(\w+) (\d+), (\d+)/i';
$replacement = '${1}1,$3';
var_dump (preg_replace($pattern, $replacement, $string));
var_dump (preg_replace('/(\w+) (\d+), (\d+)/i', $replacement, $string));


$string = 'The quick brown fox jumped over the lazy dog.';
$patterns = array();
$patterns[0] = '/quick/';
$patterns[1] = '/brown/';
$patterns[2] = '/fox/';
$replacements = array();
$replacements[2] = 'bear';
$replacements[1] = 'black';
$replacements[0] = 'slow';
var_dump (preg_replace($patterns, $replacements, $string));

ksort($patterns);
ksort($replacements);
var_dump (preg_replace($patterns, $replacements, $string));


$patterns = array ('/(19|20)(\d{2})-(\d{1,2})-(\d{1,2})/',
                   '/^\s*{(\w+)}\s*=/');
$replace = array ('\3/\4/\1\2', '$\1 =');
echo preg_replace($patterns, $replace, '{startDate} = 1999-5-27');


$str = 'foo   o';
$str = preg_replace('/\s\s+/', ' ', $str);
// This will be 'foo o' now
echo $str;


$count = 5;
echo preg_replace(array('/\d/', '/\s/'), '*', 'xp 4 to', -1 , $count);
echo $count;


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

$keywords = preg_split("/[\s,]+/", "hypertext language, programming");
print_r($keywords);

$str = 'string';
$chars = preg_split('//', $str, -1, PREG_SPLIT_NO_EMPTY);
print_r($chars);

$str = 'hypertext language programming';
$chars = preg_split('/ /', $str, -1, PREG_SPLIT_OFFSET_CAPTURE);
print_r($chars);

var_dump (preg_replace ('~a|~', 'b', 'a'));
var_dump (preg_replace ('~a|~', 'a', 'b'));

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

foreach (array('2006-05-13', '06-12-12', 'data: "12-Aug-87"') as $s) {
  var_dump(preg_match('~
    (?P<date> 
    (?P<year>(\d{2})?\d\d) -
    (?P<month>(?:\d\d|[a-zA-Z]{2,3})) -
    (?P<day>[0-3]?\d))
  ~x', $s, $m));

  var_dump($m);
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

var_dump(preg_replace(array('@//.*@', '@/\*.*\*/@sU'), array('', 'preg_replace("/[^\r\n]+/", "", \'$0\')'), "hello\n//x \n/*\ns\n*/"));

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


/**
 * @kphp-required
 * @param string[] $param
 * @return string
 */
function cb($param) {
  var_dump($param);
  return "yes!";
}

#var_dump(preg_replace('', array(), ''));

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

$foo = 'bla bla bla';

var_dump(preg_match('/(?<!\w)(0x[\p{N}]+[lL]?|[\p{Nd}]+(e[\p{Nd}]*)?[lLdDfF]?)(?!\w)/', $foo, $m));
var_dump($m);

$subject = '//abcde';
var_dump(preg_match('@^(/([a-z]*))*$@', $subject, $m)); var_dump($m);
var_dump(preg_match('@^(/(?:[a-z]*))*$@', $subject, $m)); var_dump($m);

$subject = '/a/abcde';
var_dump(preg_match('@^(/([a-z]+))+$@', $subject, $m)); var_dump($m);
var_dump(preg_match('@^(/(?:[a-z]+))+$@', $subject, $m)); var_dump($m);

$pattern =
"/\s([\w_\.\/]+)(?:=([\'\"]?(?:[\w\d\s\?=\(\)\.,'_#\/\\:;&-]|(?:\\\\\"|\\\')?)+[\'\"]?))?/";
$context = "<simpletag an_attribute=\"simpleValueInside\">";

$match = array();

if ($result = preg_match_all($pattern, $context, $match))
{
var_dump($result);
var_dump($match);
}

$regex = '/(insert|drop|create|select|delete|update)([^;\']*('."('[^']*')+".')?)*(;|$)/i';

$sql = 'SELECT * FROM #__components';

if (preg_match($regex, $sql, $m)) echo 'matched';
else echo 'not matched';

/*
mimics_pcre fail :( 
print_r($m);
*/



var_dump(preg_replace(array('/\da(.)/ui', '@..@'), '$1', '12Abc'));
var_dump(preg_replace(array('/\da(.)/ui', '@(.)@'), '$1', array('x','a2aA', '1av2Ab')));


var_dump(preg_replace(array('/[\w]+/'), array('$'), array('xyz', 'bdbd')));
var_dump(preg_replace(array('/\s+/', '~[b-d]~'), array('$'), array('x y', 'bd bc')));


var_dump(preg_match('/\d+/', '123 456 789 012', $match, 0));
var_dump($match);

var_dump(preg_match('/\d+/', '123 456 789 012', $match, 0));
var_dump($match);

var_dump(preg_match_all('/\d+/', '123 456 789 012', $match, 0));
var_dump($match);


var_dump(preg_split('/PHP_(?:NAMED_)?(?:FUNCTION|METHOD)\s*\((\w+(?:,\s*\w+)?)\)/', "PHP_FUNCTION(s, preg_match)\n{\nlalala", -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_OFFSET_CAPTURE));

$data = '(#11/19/2002#)';
var_dump(preg_split('/\b/', $data));


var_dump(preg_split('/[\s, ]+/', 'x yy,zzz'));
var_dump(preg_split('/[\s, ]+/', 'x yy,zzz', -1));
var_dump(preg_split('/[\s, ]+/', 'x yy,zzz', 0));
var_dump(preg_split('/[\s, ]+/', 'x yy,zzz', 1));
var_dump(preg_split('/[\s, ]+/', 'x yy,zzz', 2));

var_dump(preg_split('/\d*/', 'ab2c3u'));
var_dump(preg_split('/\d*/', 'ab2c3u', -1, PREG_SPLIT_NO_EMPTY));


var_dump(preg_split('/(\d*)/', 'ab2c3u', -1, PREG_SPLIT_DELIM_CAPTURE));
var_dump(preg_split('/(\d*)/', 'ab2c3u', -1, PREG_SPLIT_OFFSET_CAPTURE));
var_dump(preg_split('/(\d*)/', 'ab2c3u', -1, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE));
var_dump(preg_split('/(\d*)/', 'ab2c3u', -1, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_OFFSET_CAPTURE));
var_dump(preg_split('/(\d*)/', 'ab2c3u', -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_OFFSET_CAPTURE));
var_dump(preg_split('/(\d*)/', 'ab2c3u', -1, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_OFFSET_CAPTURE));





/*
PHP 5.2.0 - 5.3.6 bug
$text = '[CODE]&lt;td align=&quot;$stylevar[right]&quot;&gt;[/CODE]';
$result = preg_replace(array('#\[(right)\](((?R)|[^[]+?|\[)*)\[/\\1\]#siU', '#\[(right)\](((?R)|[^[]+?|\[)*)\[/\\1\]#siU'), '', $text);
var_dump($text);
var_dump($result);

$result = preg_replace('#\[(right)\](((?R)|[^[]+?|\[)*)\[/\\1\]#siU', '', $text);
var_dump($text);
var_dump($result);
*/







$input = "plain [indent] deep [indent] [abcd]deeper[/abcd] [/indent] deep [/indent] plain"; 

/**
 * @param mixed $input
 * @return string
 */
function parseTagsRecursive($input)
{
	global $count; 
    $regex = '#\[indent]((?:[^[]|\[(?!/?indent])|(?R))+)\[/indent]#';

    if (is_array($input)) {
        $input = '<div style="margin-left: 10px">'.$input[1].'</div>';
    }


    $res = preg_replace_callback($regex, 'parseTagsRecursive', $input, -1, $count);
    var_dump ($count);
    return (string)$res;

}

$output = parseTagsRecursive($input);

echo $output, "\n";


/**
 * @kphp-required
 * @param string[] $x
 * @return string
 */
function g($x) {
	return "'{$x[0]}'";
}

var_dump(preg_replace_callback('@\b\w{1,2}\b@', 'g', array('a b3 bcd', 'v' => 'aksfjk', 12 => 'aa bb')));

@var_dump(preg_replace_callback('~\A.~', 'g', array(array('xyz'))));

/**
 * @kphp-required
 * @param string[] $m
 * @return string
 */
function tmp($m) {
  return strtolower($m[0]);
}

var_dump(preg_replace_callback('~\A.~', 'tmp', 'ABC'));

var_dump(preg_replace_callback("/(ab)(cd)(e)/", "cb", 'abcde'));


