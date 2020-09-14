@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

$x = [new Classes\A];
if (1) {
    $x = 5;
}
echo $x;
