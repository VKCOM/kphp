<?php

namespace Classes;

class F
{
    /** @var int[] */
    var $intArr = [];
    /** @var string[] */
    var $stringArr = [];

    /**
     * @kphp-infer
     * @param int $v
     */
    public function appendInt($v) {
        $this->intArr[] = $v;
    }

    /**
     * @kphp-infer
     * @param string $v
     */
    public function appendString($v) {
        $this->stringArr[] = $v;
    }
}
