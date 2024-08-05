@ok k2_skip
<?php
  for ($i = 0; $i < 256; $i++)
    $s_full .= chr ($i);
  var_dump (addcslashes ($s_full, "\0..\xff"));
  var_dump (addcslashes ($s_full, " \n\r\t\v\0"));
  var_dump (addcslashes ($s_full, ""));
  var_dump (addslashes ($s_full));
  var_dump (stripslashes (addslashes ($s_full)));
  var_dump (stripslashes (" \n\r\t\v\0"));
  
  $s = str_repeat ($s_full, 100);

  $s = addslashes ($s).'\n\r\b\a\f\q\1\012\0\\\\\\';

  for ($i = 0; $i < 100; $i++) {
    $res = stripslashes ($s);
  }

  var_dump ($res);

$tests = [
  '',
  '\H\e\l\l\o \W\or\l\d',
  'Hello World\\r\\n',
  '\\\Hello World',
  '\x48\x65\x6c\x6c\x6f \x57\x6f\x72\x6c\x64',
  '\x1\x2',
  '\x11\x22',
  '\x1\x22',
  '\x11\x2',
  '\xfff12',
  '\xz12\x_112',
  '\x',
  '\xx',
  '\\x',
  '\x1',
  '\xrr',
  "\x\n",
  '\\',
  '\'',
  "\\\"",
  "\\",
  "\\\\\"",
  '\110\145\154\154\157 \127\157\162\154\144',
  '\\a',
  '\\b',
  '\\f',
  '\\t',
  '\\v',
  '\065\x64',
  '\0',
  '\1a\1',
  '\11a\11',
  '\1111',
  '\9a',
  'hello' . chr(0) . 'world',
   chr(0) . 'hello' . chr(0),
   chr(0) . chr(0) . 'hello',
   chr(0),
   chr(65),
];

$all_tests = [];
foreach ($tests as $test) {
  $all_tests[] = $test;
  $all_tests[] = "$test$test";
  $all_tests[] = "$test\n\\$test\\";
  $all_tests[] = "$test ";
  $all_tests[] = " $test";
  $all_tests[] = "$test\x00";
  $all_tests[] = "\x00$test";
  $all_tests[] = "$test\x00$test";
}

foreach ($all_tests as $test) {
  $res = stripcslashes($test);
  var_dump($res);
  $res2 = addcslashes($res, " \n\r\t\v\0");
  var_dump(stripcslashes($res2));
}

var_dump(bin2hex(stripcslashes('\\a')));
var_dump(bin2hex(stripcslashes('\\b')));
var_dump(bin2hex(stripcslashes('\\f')));
var_dump(bin2hex(stripcslashes('\\t')));
var_dump(bin2hex(stripcslashes('\\v')));
