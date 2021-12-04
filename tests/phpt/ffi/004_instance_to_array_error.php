@kphp_should_fail
KPHP_ENABLE_FFI=1
/Called instance_to_array\(\) with CData/
<?php

$cdef = FFI::cdef('
  struct Foo {
    int32_t x;
    int16_t y;
    const char *s;
  };
');

$foo = $cdef->new('struct Foo');

var_dump(instance_to_array($foo));
