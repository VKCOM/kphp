<?php

namespace Classes;

class A {
    /** @var int */
    var $magic = 0;

    /**
     * @param int $magic
     */
    public function __construct($magic = 0) {
        $this->magic = $magic;
    }

    public function print_magic() {
        var_dump("A: {$this->magic}");
    }
}
