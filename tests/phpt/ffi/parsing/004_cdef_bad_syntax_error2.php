@kphp_should_fail
/FFI::cdef\(\): line 5: syntax error, unexpected end of file, expecting ; or \( or \* or IDENTIFIER/
<?php

// Missing semicolon after a struct declaration.

$cdef = FFI::cdef('
  struct Foo {
    int8_t x;
  }
');
