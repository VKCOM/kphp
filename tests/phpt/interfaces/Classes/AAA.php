<?php

namespace Classes;

/** @kphp-immutable-class */
class AAA {
    public $x = 1;

    /**
     * @return AAA
     */
    public static function create() {
      return new AAA();
    }
}

