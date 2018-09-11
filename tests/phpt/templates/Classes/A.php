<?php

namespace Classes;

class A {
    /** @var int */
    var $magic = 0;

    public function print_magic() {
        var_dump("A: {$this->magic}");
    }
}
