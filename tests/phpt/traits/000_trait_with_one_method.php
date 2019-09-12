@ok
<?php

trait GetJSON {
    public function get_json() {
        return "{'x' : ".$this->x.", 'y' : ".$this->y."}";
    }
}

class A {
    use GetJSON;

    public $x = 10;
    public $y = 2;
}

$x = new A();
var_dump($x->get_json());
