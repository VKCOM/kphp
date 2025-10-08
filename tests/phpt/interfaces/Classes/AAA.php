<?php

namespace Classes;

/** @kphp-immutable-class
 *  @kphp-serializable */
class AAA {
    /** @kphp-serialized-field 0 */
    public $x = 1;

    /**
     * @return AAA
     */
    public static function create() {
      return new AAA();
    }
}

