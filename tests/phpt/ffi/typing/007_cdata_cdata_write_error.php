@kphp_should_fail
<?php

$int = FFI::new('int');
$int->cdata->cdata = 10;
