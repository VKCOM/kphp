<?php

namespace Classes;

use Classes\A;

class B {
    public A $a;
    public function __construct(A $a) {
        $this->a = $a;
    }

    public function __toString() {
        return strval($this->a);
    }
}
