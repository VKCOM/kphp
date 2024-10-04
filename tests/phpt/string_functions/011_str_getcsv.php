@ok k2_skip
<?php

$s1 = <<<_STR
"a","b","c"
_STR;

$s2 = <<<_STR
*a*,*b*,*\*c*
_STR;


// In php empty delimiter and enclosure args leads to the same behavior as omitted args
var_dump(str_getcsv($s1));
var_dump(str_getcsv($s1, ""));

var_dump(str_getcsv($s1, ","));
var_dump(str_getcsv($s1, ",", ""));

// But empty escape symbol has same semantics as one backslash ("\")
// 1 <=> 2
// not 1 <=> 3
var_dump(str_getcsv($s2, ",", "*"));        // 1
var_dump(str_getcsv($s2, ",", "*", "\\"));  // 2
var_dump(str_getcsv($s2, ",", "*", ""));    // 3


