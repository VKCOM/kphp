@kphp_should_fail
/yield isn't supported/
<?php
function foo() {
    foreach (range(1, 3) as $i) {
        yield;
    }
}

var_dump(foo());
