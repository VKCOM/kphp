@kphp_should_fail
/pass int\[\] to argument \$a of function\(\$a\)/
<?php


class A {
    function f() { echo "A f\n"; }
}

/**
 * @param A[] $a
 */
$f2 = function($a) {  };
$f2([1]);
