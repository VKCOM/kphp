<?php

namespace Classes;

/** 
 * @kphp-serializable
 **/
class B {
    /**
     * @kphp-serialized-field 2
     * @var null
     */
    public $null_f = null;

    public function equal_to(B $another): bool {
        var_dump($another->null_f);
        return $this->null_f === $another->null_f;
    }
}
