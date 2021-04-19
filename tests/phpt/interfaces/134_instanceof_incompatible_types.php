@kphp_should_fail
/A and B are not compatible/
<?php

class A {}
class B {}

function foo(A $a) {
    if ($a instanceof B) {
    }
}

foo(new A());
