@kphp_should_fail
/pass A to argument \$x of check/
/but it's declared as @param mixed/
<?php
require_once 'kphp_tester_include.php';

class A {};

function check(mixed $x) {
    var_dump($x);
}

function main() {
    check(to_mixed(new A)); // OK
    check(new A()); // Error
}

main();
