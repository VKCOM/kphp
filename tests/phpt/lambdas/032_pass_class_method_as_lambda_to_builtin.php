@ok k2_skip
<?php

define('A_method', 'increment_by_value');

class A {
    public $value = 10;
    const METHOD_NAME = 'increment_by_value';

    /**
     * @param int $x
     * @return int
     */
    public function increment_by_value($x) {
        return $x + $this->value;
    }
}

$instance = new A();
$res = [1, 2, 3];

$res = array_map([$instance, "increment_by_value"], $res);
var_dump($res);

$res = array_map([$instance, A_method], $res);
var_dump($res);

$res = array_map([$instance, A::METHOD_NAME], $res);
var_dump($res);
