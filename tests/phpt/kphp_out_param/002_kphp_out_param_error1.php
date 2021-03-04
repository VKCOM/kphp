@kphp_should_fail
/@kphp-out-param expects a \$varname argument/
<?php

/**
 * @kphp-out-param
 * @param int $x
 */
function out_param(&$x) {}

out_param($v);
