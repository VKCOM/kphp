@kphp_should_fail
/left operand of 'instanceof' should be an instance or mixed, but passed int/
<?php

class A { public $x = 10; }
$x = 10;
if ($x instanceof A) {
    var_dump("ERROR");
} else {
    var_dump("OK");
}

