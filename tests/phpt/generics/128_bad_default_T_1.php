@kphp_should_fail
/withDef3\/\*<A>\*\/\(null\);/
/Calculated generic <T> = A breaks condition 'T:I'/
/It does not implement I/
<?php

interface I { function f(); }

class A { function f() { echo "A f\n"; } }


/**
 * @kphp-generic T: I = I
 * @param ?T $i
 */
function withDef3($i) {
    if ($i === null) echo "null\n";
    else $i->f();
}

withDef3(null);
withDef3/*<A>*/(null);
