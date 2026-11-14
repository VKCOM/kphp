@kphp_should_fail
/Cannot call count\(\) with non-countable type int/
/Cannot call count\(\) with non-countable type float/
/Cannot call count\(\) with non-countable type bool/
/Cannot call count\(\) with non-countable type string/
<?php

function test_count(int $i, float $f, bool $b, string $s) {
    var_dump(count($i));
    var_dump(count($f));
    var_dump(count($b));
    var_dump(count($s));
}

test_count(42, 1.5, time() > 0, "text");
