@kphp_should_fail
/static FFI::new\(\) can only create scalar types/
<?php

$foo = FFI::new('struct Foo');
