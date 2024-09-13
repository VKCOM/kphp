@kphp_should_fail k2_skip
/pass \?string to argument \$s of ensure_string/
<?php

function ensure_string(string $s) {}

function test() {
  $arr = FFI::new('const char*[2]');
  ensure_string(ffi_array_get($arr, 0));
}

test();
