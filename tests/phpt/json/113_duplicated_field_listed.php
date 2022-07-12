@kphp_should_fail
/class M3 {/
/@kphp-json 'fields' lists a duplicated item \$f1/
<?php

/**
 * @kphp-json fields=$f1, f1
 */
class M3 {
    public int $f1 = 0;
}


echo JsonEncoder::encode(new M3) . PHP_EOL;

