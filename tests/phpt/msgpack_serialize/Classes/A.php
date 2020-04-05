<?php

namespace Classes;

use Classes2\A as A2;

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @kphp-serialized-field 1
     * @var A2
     */
    public $a2_f = null;

    /**
     * @kphp-serialized-field 2
     * @var B
     */
    public $b_f = null;

    public function __construct() {
        $this->a2_f = new A2();
        $this->b_f = new B();
    }

    public function equal_to(A $another): bool {
        var_dump($another->a2_f->int_f);
        var_dump($another->b_f->null_f);
        return $this->a2_f->equal_to($another->a2_f)
            && $this->b_f->equal_to($another->b_f);
    }
}
