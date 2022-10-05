<?php

namespace WithMagic009;

class WithToString009 {
    private int $n;

    public function __construct(int $n) {
        $this->n = $n;
    }

    public function __toString() {
        \Logic009\TestMagic009::doSmth();
        return "n=$this->n";
    }
}
