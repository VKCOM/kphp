@kphp_should_fail
/Unpacking an argument to a non-variadic param/
<?php

function f($a ::: array) {
}
f(...[[1, 2]]);

