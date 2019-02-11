@ok
<?php

interface iA {
    public function do_a($a, $b);
}

class A implements iA {
    public function do_a($a, $b)
    {
        var_dump($a, $b);
    }
}

var_dump(10);
