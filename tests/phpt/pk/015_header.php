@ok
<?php

header('X-TEST-1: value1');
header('X-TEST-1: value2');
header('X-TEST-2: value1', false);
header('X-TEST-2: value2', false);
header('X-TEST-3: value1', false);
header('X-TEST-3: value2', false);
header('X-TEST-3: value3', true);
header('Date: haha, date in tests');
setcookie('test-cookie', 'blabla');
$x = headers_list();
foreach ($x as $_ => &$v) {
    $v = strtolower($v);
}
sort($x);


#ifndef KPHP
// not working in console php
$x = [
    'content-type: text/html; charset=windows-1251',
    'date: haha, date in tests',
    'server: nginx/0.3.33',
    'set-cookie: test-cookie=blabla',
    'x-test-1: value2',
    'x-test-2: value1',
    'x-test-2: value2',
    'x-test-3: value3'
];
#endif

var_dump($x);
