@kphp_should_fail
KPHP_ENABLE_FFI=1
/FFI::cdef\(\): line 3: syntax error, unexpected IDENTIFIER/
<?php

// Using struct as a param without 'struct' keyword.

$cdef = FFI::cdef('
  struct Foo { int8_t x; };
  void f(Foo foo);
');
