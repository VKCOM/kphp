<?php

namespace Classes2;

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $int_f = 11;

    public function equal_to(A $another): bool {
        var_dump($another->int_f);
        return $this->int_f === $another->int_f;
    }
}
