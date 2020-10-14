@kphp_should_fail
<?php

interface IA {
    /**
     * @param mixed $x
     * @return int
     */
    public function foo($x);
}

class A implements IA {
    /**
     * @param int $x
     * @return int
     */
    public function foo($x) {
        return 10;
    }
}

$a = function (IA $ia) { return $ia->foo(10); };
$a(new A());

