@kphp_should_fail
/getA2\(\)->f\(\);/
/getA2\(\) does not return an instance or it can't be detected/
<?php

class A { function f() { echo "A f\n"; } }

function getA1() { $a = getA2(); return $a; }
function getA2() { $a = getA1(); return $a; }

getA2()->f();
