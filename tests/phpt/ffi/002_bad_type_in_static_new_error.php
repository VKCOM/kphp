@kphp_should_fail k2_skip
/static FFI::new\(\) can only create scalar types/
<?php

$foo = FFI::new('struct Foo');
