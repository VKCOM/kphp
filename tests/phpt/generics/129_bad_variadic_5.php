@kphp_should_fail
/in vg1\<\.\.\.TV1>/
/Invalid @param declaration for a variadic generic/
/Must declare @param TV1 \.\.\.\$args/
/in vg2<\.\.\.TV2>/
/Must declare @param TV2 \.\.\.\$args/
<?php

/**
 * @kphp-generic ...TV1
 * @param int... $args
 */
function vg1(...$args) {
    echo count($args);
}

/**
 * @kphp-generic ...TV2
 */
function vg2(...$args) {
    echo count($args);
}

vg1(1, 's', [1]);
vg2(1, 's', [1]);
