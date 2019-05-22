@ok
<?php

class A {
    public $value = 10;
    const Name = 'increment_by_value';

    public function increment_by_value($x) {
        return $x + $this->value;
    }
}

$instance = new A();
$res = [1, 2, 3];

$res = array_map([$instance, "__construct"], $res);
var_dump($res);
