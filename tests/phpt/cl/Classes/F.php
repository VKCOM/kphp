<?php

namespace Classes;

class F
{
    /** @var int[] */
    var $intArr = [];
    /** @var string[] */
    var $stringArr = [];

    public function appendInt($v) {
        $this->intArr[] = $v;
    }

    public function appendString($v) {
        $this->stringArr[] = $v;
    }
}
