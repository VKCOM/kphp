<?php

namespace Classes;

class IntHolder
{
    var $a = 10;

    /**
     * @kphp-infer
     * @param int $a
     */
    public function __construct($a = 0) {
        $this->a = $a;
    }

    /**
     * @kphp-required
     * @kphp-infer
     * @param int $x
     * @return int
     */
    public static function f_static($x) { 
        var_dump("IntHolder::f_static: $x");
        return $x;
    }
}
