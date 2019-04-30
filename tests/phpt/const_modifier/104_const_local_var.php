@kphp_should_fail
/Modification of const/
<?php

function foo($x) {
    /** @kphp-const $x */
    $x = 20;

    $x = 30;
}

foo(20);
