@ok
<?php

interface iWithVariadics {
    public function print_me($x, $y, ...$args);
}

class A implements iWithVariadics {
    public function print_me($x, $y, ...$args)
    {
        var_dump($args);
    }
}

(new A)->print_me(1, 2, 3, 4, "asdf", [1, 2, 3]);

