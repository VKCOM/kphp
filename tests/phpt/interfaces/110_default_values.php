@kphp_should_fail
/default value of interface parameter/
<?php

interface WithDefault2 {
    public function foo($x, $y = 10 + 10);
}

class B implements WithDefault2 {
    public function foo($x = 20, $y = 20) { var_dump($x, $y); }
}


$b = new B();
$b->foo();
