@kphp_should_fail
/FFI::cdef\(\): line 3: syntax error, unexpected IDENTIFIER/
<?php

// Using struct as a param without 'struct' keyword.

$cdef = FFI::cdef('
  struct Foo { int8_t x; };
  void f(Foo foo);
');
