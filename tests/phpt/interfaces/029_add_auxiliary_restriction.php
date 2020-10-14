@ok
<?php

interface IA {
    /**
     * @param int $x
     * @return int
     */
    public function foo($x);
}


class A implements IA {
    /**
     * @param mixed $x
     * @param string $y
     * @return int
     */
    public function foo($x, $y = "asdf") {
        return 10;
    }
}

$a = function(IA $ia) { var_dump($ia->foo(10)); };
$a(new A());
