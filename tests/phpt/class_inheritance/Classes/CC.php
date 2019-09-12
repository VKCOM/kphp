<?php

namespace Classes;

abstract class CC extends BB {
    public function foo() {
        var_dump("C");
    }

    abstract public function bar();
}

