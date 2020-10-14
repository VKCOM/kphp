@kphp_should_fail
<?php

interface IA {
    /**
     * @param int $x
     * @param mixed $y
     * @return int
     */
    public function foo($x, $y);
}

class A implements IA {
    /**
     * @param int $x
     * @param int $y
     * @param string $z
     * @return int
     */
    public function foo($x, $y, $z = "asdf") {
        return 10;
    }
}

$a = function(IA $ia) { $ia->foo(10, 20); };
$a(new A());
