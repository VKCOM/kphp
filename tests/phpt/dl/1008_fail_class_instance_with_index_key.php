@kphp_should_fail
<?php
class A { 
    public $x = 20; 

    /**
     * @kphp-infer
     * @param int $x
     */
    public function __construct($x = 20) {
        $this->x = $x;
    }
}

$a = [[new A(), new A()], [new A(), new A()]];
$b = array_column($a, 0, 0); 
