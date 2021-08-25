@kphp_should_fail
KPHP_ENABLE_FFI=1
/static FFI::new\(\) can only create scalar types/
<?php

$foo = FFI::new('struct Foo');
