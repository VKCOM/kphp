@kphp_should_fail
KPHP_ENABLE_FFI=1
<?php

$int = FFI::new('int');
$int->cdata->cdata = 10;
