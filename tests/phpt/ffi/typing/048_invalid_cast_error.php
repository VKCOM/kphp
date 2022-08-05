@kphp_should_fail
/Could not find class/
<?php

$int8 = FFI::new('int8_t');
$_ = FFI::cast('struct Foo', $int8);
