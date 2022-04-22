@kphp_should_fail
/pass int to argument \$instance of instance_cast/
<?php

class A { public $x = 10; }
$x = 10;
if ($x instanceof A) {
    var_dump("ERROR");
} else {
    var_dump("OK");
}

