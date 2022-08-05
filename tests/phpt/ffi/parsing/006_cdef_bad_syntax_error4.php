@kphp_should_fail
/FFI::cdef\(\): line 2: syntax error, unexpected \{/
<?php

// Function bodies are forbidden.

$cdef = FFI::cdef('
  void f() {}
');
