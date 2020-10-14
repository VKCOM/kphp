<?php

namespace Classes;

class F
{
    /** @var int[] */
    var $intArr = [];
    /** @var string[] */
    var $stringArr = [];

    /**
     * @param int $v
     */
    public function appendInt($v) {
        $this->intArr[] = $v;
    }

    /**
     * @param string $v
     */
    public function appendString($v) {
        $this->stringArr[] = $v;
    }
}
