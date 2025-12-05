@ok
<?php

$res = preg_split('/a/', "ac", -1, PREG_SPLIT_NO_EMPTY);
var_dump($res);

$x = "123|(123|123)&!123|123";
$res = preg_split('/(1)2(3)/', $x, -1,
                  PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY);
var_dump($res);

$x = "10067|(10067|10068)&!10060|581";
$res = preg_split('/(\d+)/', $x, -1,
                  PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY);
var_dump($res);
