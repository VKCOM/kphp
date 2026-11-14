@kphp_should_fail
/Cannot call sizeof\(\) with non-countable type int/
<?php

function test_sizeof(int $value) {
    var_dump(sizeof($value));
}

test_sizeof(42);
