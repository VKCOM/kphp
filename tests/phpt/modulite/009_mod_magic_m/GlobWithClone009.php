<?php

class GlobWithClone009 {
    private $was_cloned = false;

    public function __clone() {
        $this->was_cloned = true;
    }

    public function printInfo() {
        echo "WithClone009 ", ($this->was_cloned ? "cloned" : "orig"), "\n";
    }
}
