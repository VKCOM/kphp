@kphp_should_fail
<?php
require_once 'Classes/autoload.php';

$x = [new Classes\A];
if (1) {
    $x = 5;
}
echo $x;
