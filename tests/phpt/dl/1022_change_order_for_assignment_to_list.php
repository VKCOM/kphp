@ok k2_skip
<?php
// dl/453_list.php
// это про порядок присвоения в элементы листа
// в PHP7 сначала присвоится в $arr[0], потом в $arr[1], в 5.6 наоборот поэтому собственно var_dump по-разному работает
$arr = [];
list($arr[0], $arr[1]) = [1, 2];
var_dump($arr);

$test = '1:2:3';
$gift = array();
list($gift['from_id'], $gift['gift_number'], $gift['id']) = explode(':', $test);

var_dump (explode(':', $test));
var_dump ($gift);
