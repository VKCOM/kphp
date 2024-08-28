@kphp_should_fail k2_skip
/Field \$badname not found in class/
<?php

$cdef = FFI::cdef('struct Foo { bool field; };');

$foo = $cdef->new('struct Foo');
$foo->badname;
