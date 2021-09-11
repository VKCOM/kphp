@kphp_should_fail php8
<?php

function foo(,) { // Free-standing commas are not allowed
    echo $arg . "\n";
}
