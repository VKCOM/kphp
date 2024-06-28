@kphp_should_fail
/yield isn't supported/
<?php
function foo($input) {
    $id = 1;
    yield $id => $input;
}

foreach(foo(["value"]) as $id => $f) {
    // ...
}
