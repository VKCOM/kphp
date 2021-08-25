<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_vector_lib();

$lib = FFI::scope('vector');
var_dump($lib->vectorlib_version);
var_dump($lib->default_value);

$arr1 = $lib->new('struct Vector2Array');
$lib->vector2_array_alloc(FFI::addr($arr1), 2);
print_vec_array($arr1);

$lib->default_value = 1.5;
var_dump($lib->default_value);

$arr2 = $lib->new('struct Vector2Array');
$lib->vector2_array_alloc(FFI::addr($arr2), 3);
print_vec_array($arr2);

$lib->vector2_array_free(FFI::addr($arr1));
$lib->vector2_array_free(FFI::addr($arr2));
