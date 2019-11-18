@ok
<?php

class A {
    public $x = true ? 10 : "a";
}

function dump($x, A $a) {
    $a->x = 20;
    var_dump($x);
}

function run() {
    $a = new A();
    dump($a->x, $a);
}

run();
