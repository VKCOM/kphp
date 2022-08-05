@kphp_should_fail
/\$ptr argument is FFI\\CData_uint16\*, expected a C string compatible type/
<?php

$v = FFI::new('uint16_t');
$s = FFI::string(FFI::addr($v));
