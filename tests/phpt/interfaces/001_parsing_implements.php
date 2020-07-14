@ok
<?php

interface iA {
    public function do_a(int $a, int $b);
}

class A implements iA {
    public function do_a(int $a, int $b)
    {
        var_dump($a, $b);
    }
}

var_dump(10);
