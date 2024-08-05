@kphp_should_fail
\Links in list() isn't supported\
<?php
$arr = array(1, array(2));
list(&$a, list(&$b)) = $arr;
var_dump($a, $b);
var_dump($arr);
