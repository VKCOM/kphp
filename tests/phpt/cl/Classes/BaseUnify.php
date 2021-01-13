<?php

namespace Classes;

use Classes\ResultNs\Result;

abstract class BaseUnify {
    /**
     * @param Result $r
     * @return Result[]|false
     */
    abstract public function unify($r);
}
