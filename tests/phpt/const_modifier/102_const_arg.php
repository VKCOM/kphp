@kphp_should_fail
/Modification of const/
<?php

/**
 * @kphp-const $x
 */
function foo(int $x) {
    $x = 20;
}

foo(20);
