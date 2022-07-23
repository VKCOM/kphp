@kphp_should_fail
/function withDef4\(\$i\) \{/
/Can't parse generic declaration <T>: ':' should go before a default value/
<?php

interface I { function f(); }

/**
 * @kphp-generic T = I : I
 * @param T $i
 */
function withDef4($i) {
    $i->f();
}

withDef4(null);
