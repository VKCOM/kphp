@kphp_should_fail
<?php

interface I { public function foo($x); };
class A implements I { public function foo($x, $y, $z = 10) { } }
