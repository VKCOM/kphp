@kphp_should_fail
/Using \$this in a static lambda/
<?php

class A {
    public $x = 20;
    public function foo() {
        $f = static function() { return $this->x; };
        var_dump($f());
    }
}

$a = new A();
$a->foo();
