@kphp_should_fail
\Сan not parse first class callable syntax\
<?php
class A {
    public true $t;
    public function __construct($t) {
        $this->t = $t;
    }

    public function getVariable(): true {
    	return $this->t;
    }
}

/** @var true */
$t = true;
$obj = new A($t);
var_dump($obj->getVariable());
