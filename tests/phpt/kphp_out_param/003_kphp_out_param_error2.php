@kphp_should_fail
/Function out_param annotated a non-existing param with @kphp-out-param/
<?php

/**
 * @kphp-out-param $y
 * @param int $x
 */
function out_param(&$x) {}

out_param($v);
