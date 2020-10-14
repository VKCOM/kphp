<?php

namespace Classes;

class TestFieldClass extends TestFieldClassParent {
    /**
     * @return string
     */
    static function getParentClass() {
        return parent::class;
    }
}

