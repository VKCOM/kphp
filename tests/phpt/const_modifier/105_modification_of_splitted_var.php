@kphp_should_fail
/Modification of const/
<?php

function foo() {
    /** @kphp-const */
    $x = 10;

    $x = "asdfasdf";

    var_dump($x);
}

foo();
