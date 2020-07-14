<?php

namespace Classes;

class AnotherIntHolder
{
    var $a = 10;

    /**
     * @kphp-infer
     * @param int $a
     */
    public function __construct($a = 0) {
        $this->a = $a;
    }
}
