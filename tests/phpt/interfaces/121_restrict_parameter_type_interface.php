@kphp_should_fail
<?php

interface IA {
    /**
     * @kphp-infer
     * @param mixed $x
     * @return int
     */
    public function foo($x);
}

class A implements IA {
    /**
     * @kphp-infer
     * @param int $x
     * @return int
     */
    public function foo($x) {
        return 10;
    }
}

$a = function (IA $ia) { return $ia->foo(10); };
$a(new A());

