<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_vector_lib();

function test() {
  $lib = FFI::scope('vector');

  $data = $lib->vector2_new_data(10);

  $array = $lib->new('struct Vector2Array');
  $array->data = $data;

  $val = $lib->new('struct Vector2');
  $val->x = 5.2;
  $val->y = 1.5;
  $lib->vector2_array_set(FFI::addr($array), 0, $val);
  $val_out = $lib->vector2_array_get(FFI::addr($array), 0);
  var_dump($val_out->x, $val_out->y);

  $lib->vector2_data_free($data);
}

test();
