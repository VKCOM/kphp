@kphp_should_fail
<?php
require_once 'polyfills.php';

$x = 5;
if (1) {
    $x = [new Classes\A];
}
echo $x;
