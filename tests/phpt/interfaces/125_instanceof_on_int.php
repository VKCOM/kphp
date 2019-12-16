@kphp_should_fail
<?php

class A { public $x = 10; }
$x = 10;
if ($x instanceof A) {
    var_dump("ERROR");
} else {
    var_dump("OK");
}

