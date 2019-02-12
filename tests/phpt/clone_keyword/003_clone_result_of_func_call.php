@ok
<?php

class A {
    public $i = 10;
}

/**
 * @return A
 */
function get_A() { return new A(); }

$arr = [new A()];

$a_copy = clone get_A();
var_dump($a_copy->i);

$a_copy->i = 1000;
var_dump($a_copy->i);

$arr[0]->i = 999;
var_dump($arr[0]->i);

$a_copy = clone $arr[0];
var_dump($a_copy->i);

$a_copy = clone new A;
var_dump($a_copy->i);


