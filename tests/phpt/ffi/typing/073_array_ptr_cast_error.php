@kphp_should_fail
/only pointers can be cast to arrays \(got uint64_t\)/
<?php

$i = FFI::new('uint64_t');
$a = FFI::cast('uint32_t[2]', $i);
