@kphp_should_fail
/yield isn't supported/
<?php
function foo() {
    for($i = 1; $i < 3; $i++) {
        yield $i;
    }
}

$a = foo();
foreach($a as $el) {
    echo $el;
}
