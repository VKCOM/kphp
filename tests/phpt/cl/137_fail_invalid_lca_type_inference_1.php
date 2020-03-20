@kphp_should_fail
<?php
require_once 'polyfills.php';

$x = [new Classes\A];
if (1) {
    $x = 5;
}
echo $x;
