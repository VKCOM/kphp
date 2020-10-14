@ok
<?php

require_once 'kphp_tester_include.php';

class Base {
    /**
     * @param int $x
     * @param tuple(string) $y
     * @return mixed[]
     */
    public function foo($x, $y) {
        return true ? [$y[0]] : [$x];
    }

    /**@return int*/
    public static function bar() {
        return 10;
    }
}

class Derived extends Base {
    public function foo($x, $y) {
        return true ? [$y[0]] : [$x];
    }

    /**@inheritDoc*/
    public static function bar() {
        return 10;
    }
}

$d = new Derived();
$d->foo(10, tuple("asd"));
Derived::bar();

$b = new Base();
$b->foo(10, tuple("asd"));
Base::bar();
