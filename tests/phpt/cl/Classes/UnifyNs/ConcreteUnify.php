<?php

namespace Classes\UnifyNs;

use Classes\BaseUnify;

class ConcreteUnify extends BaseUnify {
    function unify($r) {
        $r->value = 2;
        return [new \Classes\ResultNs\Result];
    }
}
