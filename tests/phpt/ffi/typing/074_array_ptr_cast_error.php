@kphp_should_fail k2_skip
/can't cast to multi-dimensional array/
<?php

$i = FFI::new('uint32_t*');
$a = FFI::cast('uint32_t[2][2]', $i);
