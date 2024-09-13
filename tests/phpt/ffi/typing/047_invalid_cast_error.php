@kphp_should_fail k2_skip
/FFI::cast\(\): line 1: syntax error/
<?php

$int8 = FFI::new('int8_t');
$_ = FFI::cast('badtype', $int8);
