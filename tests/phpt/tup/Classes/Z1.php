<?php

namespace Classes;

class Z1
{
    /** @var int */
    var $num = 0;
    /** @var tuple(any, string, A[]) */
    var $t;

    public function getInitial() {
        return tuple($this->num, get_class($this), [new A]);
    }

    public function setRawTuple($t) {
        $this->t = $t;
        // tuple elements are still read-only! so the pattern is useless in practice, just for tests
    }

    public function printAll() {
        echo "num = ", $this->num, "\n";
        echo "tuple int = ", $this->t[0], "\n";
        echo "tuple string = ", $this->t[1], "\n";

        /** @var A[] */
        $aArr = $this->t[2];
        foreach ($aArr as $a) {
            echo "a[i]->a = ", $a->a, "\n";
        }
    }
}
