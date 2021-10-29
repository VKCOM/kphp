@kphp_should_fail
KPHP_ENABLE_FFI=1
/line 1: syntax error, unexpected \}/
<?php

\FFI::cdef('struct {};');
