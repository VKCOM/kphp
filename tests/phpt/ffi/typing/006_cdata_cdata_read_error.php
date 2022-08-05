@kphp_should_fail
<?php

$int = FFI::new('int');
var_dump($int->cdata->cdata);
