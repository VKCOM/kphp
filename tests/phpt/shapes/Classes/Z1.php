<?php

namespace Classes;

class Z1
{
    /** @var int */
    var $num = 0;
    /** @var shape(x:int, y:string, z:A[]) */
    var $t;

    public function getInitial() {
        return shape(['x' => $this->num, 'y' => get_class($this), 'z' => [new A]]);
    }

    public function setRawShape($t) {
        $this->t = $t;
    }

    public function printAll() {
        echo "num = ", $this->num, "\n";
        echo "shape int = ", $this->t['x'], "\n";
        echo "shape string = ", $this->t['y'], "\n";

        foreach ($this->t['z'] as $a) {
            echo "a[i]->a = ", $a->a, "\n";
        }
    }
}
