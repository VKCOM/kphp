@ok
<?php

class A { public $i = 7777; }
class B { public $i = 222; }

class BHolder {
    /** @var B */
    public $data;

    public function __construct() { $this->data = new B(); }
}

class AHolder {
    /** @var A */
    public $data;

    public function __construct() { $this->data = new A(); }
}

class CHolder {
    /** @var int */
    public $data = 10;
}

/**
 * @kphp-template T $holder
 * @kphp-return T::data
 */
function get_data($holder) {
    return $holder->data;
}

/**
 * @kphp-template T $holder
 * @kphp-return T::data[]
 */
function get_arr_data($holder) {
    return [$holder->data];
}

function test_simple() {
    $a = get_data(new AHolder());
    $b = get_data(new BHolder());

    var_dump($a->i);
    var_dump($b->i);

    $c = get_data(new CHolder());
    var_dump($c);
}

function test_getting_arrays() {
    $a = get_arr_data(new AHolder());
    $b = get_arr_data(new BHolder());

    var_dump($a[0]->i);
    var_dump($b[0]->i);
}


test_simple();
test_getting_arrays();

