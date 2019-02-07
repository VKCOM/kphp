<?php

namespace Classes;

class IntHolder
{
    var $a = 10;

    public function __construct($a = 0) {
        $this->a = $a;
    }

    /**
     * @kphp-required
     */
    public static function f_static($x) { 
        var_dump("IntHolder::f_static: $x");
        return $x;
    }
}
