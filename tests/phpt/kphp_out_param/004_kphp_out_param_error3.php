@kphp_should_fail
/Function out_param annotated x as out-param, but it is not a reference/
<?php

/**
 * @kphp-out-param $x
 * @param int $x
 */
function out_param($x) {}

out_param($v);
