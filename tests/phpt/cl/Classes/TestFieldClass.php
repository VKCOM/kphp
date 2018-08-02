<?php

namespace Classes;

class TestFieldClass extends TestFieldClassParent {
    public static function getParentClass() {
        return parent::class;
    }
}

