@kphp_should_fail k2_skip
/ffi_array_get: invalid \$arr argument/
<?php

$x = 0;
ffi_array_get($x, 0);
