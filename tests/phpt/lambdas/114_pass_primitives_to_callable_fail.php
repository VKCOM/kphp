@kphp_should_fail
/Seems that something is wrong with a callable passed, it's not callable actually/
<?php

class A {}

function takeCal(callable $cb) {
    $cb();
}

takeCal(1);
takeCal(new A);
