@kphp_should_fail k2_skip
<?php
class A { 
    public $x = 20; 

    public function __construct($x = 20) {
        $this->x = $x;
    }
}

$a = [[new A(), new A()], [new A(), new A()]];
$b = array_column($a, 0, 0); 
