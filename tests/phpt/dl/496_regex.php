@ok callback benchmark k2_skip
<?php

var_dump (preg_split ('~|й~u', 'п'));
var_dump (preg_split ('~|й~u', 'й'));


$keywords = preg_split("/[\s,]+/", "hypertext language, programming");
print_r($keywords);

$str = 'string';
$chars = preg_split('//', $str, -1, PREG_SPLIT_NO_EMPTY);
print_r($chars);

$str = 'hypertext language programming';
$chars = preg_split('/ /', $str, -1, PREG_SPLIT_OFFSET_CAPTURE);
print_r($chars);

foreach (array('2006-05-13', '06-12-12', 'data: "12-Aug-87"') as $s) {
  var_dump(preg_match('~
    (?P<date> 
    (?P<year>(\d{2})?\d\d) -
    (?P<month>(?:\d\d|[a-zA-Z]{2,3})) -
    (?P<day>[0-3]?\d))
  ~x', $s, $m));

  var_dump($m);
}


$foo = 'bla bla bla';

var_dump(preg_match('/(?<!\w)(0x[\p{N}]+[lL]?|[\p{Nd}]+(e[\p{Nd}]*)?[lLdDfF]?)(?!\w)/', $foo, $m));
var_dump($m);

$subject = '//abcde';
var_dump(preg_match('@^(/([a-z]*))*$@', $subject, $m)); var_dump($m);
var_dump(preg_match('@^(/(?:[a-z]*))*$@', $subject, $m)); var_dump($m);

$subject = '/a/abcde';
var_dump(preg_match('@^(/([a-z]+))+$@', $subject, $m)); var_dump($m);
var_dump(preg_match('@^(/(?:[a-z]+))+$@', $subject, $m)); var_dump($m);

$regex = '/(insert|drop|create|select|delete|update)([^;\']*('."('[^']*')+".')?)*(;|$)/i';

$sql = 'SELECT * FROM #__components';

if (preg_match($regex, $sql, $m)) echo 'matched';
else echo 'not matched';

/*
mimics_pcre fail :( 
print_r($m);
*/


var_dump(preg_match('/\d+/', '123 456 789 012', $match, 0));
var_dump($match);

var_dump(preg_match('/\d+/', '123 456 789 012', $match, 0));
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
