@kphp_should_fail
/Unrecognized syntax after '\$cn::'/
<?php

function f() {
    $cn = 'A';
    $cn::[0];
}

f();
