@ok callback benchmark
<?php

var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '\26\25\24\23\22\21\20\19\18\17\16\15\14\13\12\11\10\9\8\7\6\5\4\3\2\1', 'abcdefghijklmnopqrstuvwxyz'));
var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '$26$25$24$23$22$21$20$19$18$17$16$15$14$13$12$11$10$9$8$7$6$5$4$3$2$1', 'abcdefghijklmnopqrstuvwxyz'));
var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '$26$25$24$23$22$21$20$19$18$17$16$15$14$13$12$11$10$9$8$7$6$5$4$3$2$1', 'abcdefghijklmnopqrstuvwxyz'));
var_dump (preg_replace ('~(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)~', '${26}${25}${24}${23}${22}${21}${20}${19}${18}${17}${16}${15}${14}${13}${12}${11}${10}${9}${8}${7}${6}${5}${4}${3}${2}${1}', 'abcdefghijklmnopqrstuvwxyz'));

var_dump (preg_replace ('~a|~', 'b', 'c'));
var_dump (preg_replace ('~a|~', 'b', 'a'));
var_dump (preg_replace ('~a|~', 'a', 'b'));

var_dump (preg_replace ('~|q~', '{\0}', 'eq'));
var_dump (preg_replace ('~|q~', '{\0}', 'ex'));

var_dump (preg_replace ('~|q~', 'w', 'e'));
var_dump (preg_replace ('~|q~', 'w', 'q'));
/* bug in PHP
var_dump (preg_replace ('~|й~u', 'п', 'р'));
*/
var_dump (preg_replace ('~|й~u', 'п', 'й'));


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


var_dump (preg_replace ('~a|~', 'b', 'a'));
var_dump (preg_replace ('~a|~', 'a', 'b'));


var_dump(preg_replace(array('@//.*@', '@/\*.*\*/@sU'), array('', 'preg_replace("/[^\r\n]+/", "", \'$0\')'), "hello\n//x \n/*\ns\n*/"));

#var_dump(preg_replace('', array(), ''));


var_dump(preg_replace(array('/\da(.)/ui', '@..@'), '$1', '12Abc'));
var_dump(preg_replace(array('/\da(.)/ui', '@(.)@'), '$1', array('x','a2aA', '1av2Ab')));


var_dump(preg_replace(array('/[\w]+/'), array('$'), array('xyz', 'bdbd')));
var_dump(preg_replace(array('/\s+/', '~[b-d]~'), array('$'), array('x y', 'bd bc')));


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
