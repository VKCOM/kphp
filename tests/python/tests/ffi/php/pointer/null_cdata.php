<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

$lib = \FFI::scope('pointers');

$result = $lib->nullptr_data();
var_dump($result === null);
var_dump($result !== null);

$intptr = \FFI::new('int*');
var_dump($intptr === null);
var_dump($intptr !== null);
