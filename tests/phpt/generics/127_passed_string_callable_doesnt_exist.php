@kphp_should_fail
/Could not find a function abc/
/Could not find a function scn\$\$mtd/
<?php

function f(callable $c) {

}

function demo1() {
    f('abc');
}
function demo2() {
    f(['scn', 'mtd']);
}

demo1();
demo2();
