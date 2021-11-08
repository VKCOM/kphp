@kphp_should_fail
KPHP_ENABLE_FFI=1
/\$ptr argument is FFI\\CData_uint16\*, expected a C string compatible type/
<?php

$v = FFI::new('uint16_t');
$s = FFI::string(FFI::addr($v));
