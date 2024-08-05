@kphp_should_fail
/yield isn't supported/
<?php
function inner() {
    yield 1;
    yield 2;
}

function foo() {
    yield 0;
    yield from inner();
    yield 3;
}

var_dump(foo());
