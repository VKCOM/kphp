@kphp_should_fail
/Incorrect return type of function: foo/
<?php

class A{ public $x = 10; }

/**
 * @return tuple(int, A)
 */
function foo() {
    return tuple(true ? 10 : "asdf", new A());
}

foo()[1]->x;
