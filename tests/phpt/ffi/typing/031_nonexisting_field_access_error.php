@kphp_should_fail
KPHP_ENABLE_FFI=1
/Field \$badname not found in class/
<?php

$cdef = FFI::cdef('struct Foo { bool field; };');

$foo = $cdef->new('struct Foo');
$foo->badname;
