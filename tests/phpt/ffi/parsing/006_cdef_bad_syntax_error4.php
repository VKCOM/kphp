@kphp_should_fail
KPHP_ENABLE_FFI=1
/FFI::cdef\(\): line 2: syntax error, unexpected \{/
<?php

// Function bodies are forbidden.

$cdef = FFI::cdef('
  void f() {}
');
