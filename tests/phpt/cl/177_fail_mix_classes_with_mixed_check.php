@kphp_should_fail
/conversion from A to mixed is forbidden/
<?php

class A { public $x = 10; }
$as = [new A()];
json_encode($as);
