@ok benchmark k2_skip
<?php
/**
 * @param string $value
 * @param string ...$args
 */
function dLog($value, ...$args) {
  $x = count($args) + 1;
  var_dump($value);
}

/**
 * @param string $value
 * @return string
 */
function inputCleanValue($value) {
  $value = "{$value}";

  // Strip slashes if not already done so.
  if (get_magic_quotes_gpc()) {
    $value = stripslashes($value);
  }
  dLog($value);

  static $patterns = array('&', '>', '<', '"', "\t", "\n", "\r", "\\", '!', "'", '$');
  static $replaces = array('&amp;', '&gt;', '&lt;', '&quot;', ' ', "\t", '', '&#092;', '&#33;', '&#39;', '&#036;');
  $value = str_replace($patterns, $replaces, $value);
  dLog($value);

  static $patterns2 = array(
    // Bad characters
    '/&(?:amp;)*#0*(?:1[01]|9|32|160|173)(?:[^0-9]|$)/s',

    // Bad unicode characters
    '/&(?:amp;)*#0*823[234568]/s',

    // Bad unicode characters in utf8
    '/\xe2\x80[\xa8-\xac\xad]/s',

    // Remove control characters
    '/[\x00\x01\x02\x07\x08\x0b-\x1f]/s',

    // Ensure unicode chars are OK
    '/&amp;#([0-9]+);/s',
  );
  //static $replaces2 = array(' ', ' ', ' ', ' ', '&#\\1;');
  //$value = preg_replace($patterns2, $replaces2, $value);
  //dLog($value);

  return $value;
}

dLog (inputCleanValue ("19177046690"));

for ($i = 0; $i < 1000; $i++) {
//  $s .= "hello";
  $s .= "a";
}
$t = $s."abacaba".$s;

for ($i = 0; $i < 100000; $i++) {
//  $res += strlen (str_replace ("helloh", "test", $s));
  $res += strlen (str_replace ("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab", "b", $s));
//  $res += strlen (str_replace ("abacaba", "testtest", $t));
}

var_dump ($res);
var_dump (substr_count ("aaaaaaaaaaaaaaaaaaaaaaaaa", "aaa", 1, 20));
var_dump (substr_count ("aaaaaaaaaaaaaaaaaaaaaaaaa", "aaa", 2, 10));
var_dump (substr_count ("aaaaaaaaaaaaaaaaaaaaaaaaa", "aaa", 3, 2));
var_dump (substr_count ("aaaaaaaaaaaaaaaaaaaaaaaaa", "aaa", 5, 5));


$str = "Hello Friend";

$arr1 = str_split($str);
$arr2 = str_split($str, 1);
$arr3 = str_split($str, 2);
$arr4 = str_split($str, 3);
$arr5 = str_split($str, 4);
$arr6 = str_split($str, 5);
$arr7 = str_split($str, 6);
$arr8 = str_split($str, 7);
$arr9 = str_split($str, 500000000);

print_r($arr1);
print_r($arr2);
print_r($arr3);
print_r($arr4);
print_r($arr5);
print_r($arr6);
print_r($arr7);
print_r($arr8);
print_r($arr9);

$text = 'asdfas {user} {friends}  sadfasdf';
$text = str_replace(array('{user}', '{friends}'), array('', "asdasd"), $text);
var_dump ($text);

$text = 'Ульянка {user} вступила в группу';
$text = str_replace(array('{user}', '{friends}'), array('', "asdasd"), $text);
var_dump ($text);

$text = 'Ульянка {user}{user}{user}{user} вступила в группу';
$text = str_replace(array('{user}', '{friends}'), array('', "asdasd"), $text);
var_dump ($text);

$text = '{user}{user}{user}{user}';
$text = str_replace(array('{user}', '{friends}'), array('', "asdasd"), $text);
var_dump ($text);

$text = '{user}';
$text = str_replace(array('{user}', '{friends}'), array('', "asdasd"), $text);
var_dump ($text);

$text = '';
$text = str_replace(array('{user}', '{friends}'), array('', "asdasd"), $text);
var_dump ($text);

$text = 'a';
$text = str_replace(array('a', '{friends}'), array('', "asdasd"), $text);
var_dump ($text);


