@kphp_should_fail
/Unpacking an argument to a non-variadic param/
<?php

function f($a, $b) {
}

f(1, ...[2]);

