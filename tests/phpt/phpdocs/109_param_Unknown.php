@kphp_should_fail
/function wrongParam1\(\$p\)/
/Variable \$p has Unknown type/
<?php

class A {}

/** @param A|int[] $p */
function wrongParam1($p) {
}

wrongParam1([]);
