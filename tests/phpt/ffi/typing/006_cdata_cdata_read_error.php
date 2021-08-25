@kphp_should_fail
KPHP_ENABLE_FFI=1
<?php

$int = FFI::new('int');
var_dump($int->cdata->cdata);
