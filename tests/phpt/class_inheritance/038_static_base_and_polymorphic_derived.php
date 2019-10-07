@ok
<?php

class Base {
    const A = 10;
}

class Derived extends Base {
    public function test() {
        var_dump(Base::A);
    }
}

$d = new Derived();
$d->test();

