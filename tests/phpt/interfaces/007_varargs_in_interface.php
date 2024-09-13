@ok k2_skip
<?php

interface iWithVariadics {
    /** @param mixed ...$args */
    public function print_me(int $x, int $y, ...$args);
}

class A implements iWithVariadics {
    /** @param mixed ...$args */
    public function print_me(int $x, int $y, ...$args)
    {
        var_dump($args);
    }
}

(new A)->print_me(1, 2, 3, 4, "asdf", [1, 2, 3]);

