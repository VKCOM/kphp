@kphp_should_fail
/return tuple<mixed , A> from foo/
<?php

class A{ public $x = 10; }

/**
 * @return tuple(int, A)
 */
function foo() {
    return tuple(true ? 10 : "asdf", new A());
}

foo()[1]->x;
