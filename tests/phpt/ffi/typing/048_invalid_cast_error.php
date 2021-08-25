@kphp_should_fail
KPHP_ENABLE_FFI=1
/Could not find class/
<?php

$int8 = FFI::new('int8_t');
$_ = FFI::cast('struct Foo', $int8);
