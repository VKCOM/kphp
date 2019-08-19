<?php

namespace Classes;

class Derived extends Base {
    public function foo() {
        parent::foo();
        var_dump("Derived");
    }
}
