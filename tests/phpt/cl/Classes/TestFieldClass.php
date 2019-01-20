<?php

namespace Classes;

class TestFieldClass extends TestFieldClassParent {
    static function getParentClass() {
        return parent::class;
    }
}

