@kphp_should_fail k2_skip
/Called to_array_debug\(\) with CData/
<?php

$cdef = FFI::cdef('
  struct Foo {
    int32_t x;
    int16_t y;
    const char *s;
  };
');

$foo = $cdef->new('struct Foo');

var_dump(to_array_debug($foo));
