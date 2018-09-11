<?php

namespace Classes;

class B {
    /** @var int */
    var $magic = 0;

    public function print_magic() {
        var_dump("B: {$this->magic}");
    }
}
