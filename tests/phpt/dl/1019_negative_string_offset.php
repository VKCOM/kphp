@ok
<?php
// dl/455_inc_xor_str.php
// https://wiki.php.net/rfc/negative-string-offsets
// В новых версиях можно по отрицательному индексу к строкам обращаться
function string_as_var($s) {
    $s[0] = 'a';
    var_dump(isset($s[-1]));
    var_dump(isset($s[-2]));
    var_dump(isset($s[-10]));
}

$a = "3896";
var_dump($a[-1]);
var_dump($a[-10]);
var_dump($a["-1"]);
string_as_var($a);

