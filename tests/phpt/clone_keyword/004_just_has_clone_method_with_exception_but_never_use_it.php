@ok k2_skip
<?php
class A {
    public function __clone() {
        throw new Exception("asdf");
    }
}

$a = new A();
