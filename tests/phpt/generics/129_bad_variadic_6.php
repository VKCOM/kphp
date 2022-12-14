@kphp_should_fail
/in vg/
/Variadic <\.\.\.TArgs> is not the last in declaration/
<?php

/**
 * @kphp-generic ...TArgs, T2
 */
function vg($t2, ...$args) {
}

vg(1, 2);

