@kphp_should_fail
/Calculated generic <T> = int breaks condition 'T:SomeI'/
/It should be a class extending SomeI/
/Calculated generic <T> = Other breaks condition 'T:SomeI'/
/tplOne2\/\*<\?Other2>\*\/\(null\);/
/Calculated generic <T2> = \?Other2 breaks condition 'T2:SomeI'/
<?php

interface SomeI { function f(); }

class Other { function f() { echo "Other f\n"; } }
class Other2 { function f() { echo "Other2 f\n"; } }

/**
 * @kphp-generic T: SomeI
 * @param T $o
 */
function tplOne($o) {
    $o->f();
}

/**
 * @kphp-generic T2: \SomeI
 * @param T2 $o
 */
function tplOne2($o) {
    $o->f();
}

tplOne(3);
tplOne(new Other);
tplOne2/*<?Other2>*/(null);
