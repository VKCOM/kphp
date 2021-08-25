<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

function test() {
  $lib = FFI::scope('pointers');
  $dst = FFI::new('int64_t');
  $lib->write_int64(FFI::addr($dst), 53);
  var_dump($dst->cdata);
}

test();
