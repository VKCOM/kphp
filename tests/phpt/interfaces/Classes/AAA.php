<?php

namespace Classes;

/** @kphp-immutable-class */
class AAA {
    public $x = 1;

    /**
     * @kphp-infer
     * @return AAA
     */
    public static function create() {
      return new AAA();
    }
}

