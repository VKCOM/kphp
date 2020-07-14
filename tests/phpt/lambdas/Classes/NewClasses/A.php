<?php

namespace Classes\NewClasses;

class A {
    /**
     * @kphp-infer
     * @param int $x
     */
    public static function f_print_static($x) {
        var_dump($x);
    }
}
