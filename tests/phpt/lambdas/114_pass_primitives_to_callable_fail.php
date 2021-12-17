@kphp_should_fail
/expected Callback1 to be callable, but any found/
/expected Callback1 to be callable, but A found/
<?php

class A {}

function takeCal(callable $cb) {
    $cb();
}

takeCal(1);
takeCal(new A);
