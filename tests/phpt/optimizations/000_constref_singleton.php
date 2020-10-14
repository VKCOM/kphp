@ok
<?php

class A {
    /**@var mixed*/
    public $x = true ? 20 : "asdf";
}

/** @return A */
function get_instance() {
    static $a = null;
    if (!$a) $a = new A();
    return $a;
}

/**
 * @param mixed $x
 */
function run($x) {
    get_instance()->x = 90;
    var_dump($x);
}

run(get_instance()->x);
