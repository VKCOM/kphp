<?php

namespace Classes;

class TestFieldClass extends TestFieldClassParent {
    /**
     * @kphp-infer
     * @return string
     */
    static function getParentClass() {
        return parent::class;
    }
}

