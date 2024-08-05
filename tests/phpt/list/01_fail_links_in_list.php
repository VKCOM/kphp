@kphp_should_fail
\Links in list() isn't supported\
<?php
$arr = ['apple', 'orange'];
list($a, &$b) = $arr;
$b = 'banana';
echo $arr[1];
