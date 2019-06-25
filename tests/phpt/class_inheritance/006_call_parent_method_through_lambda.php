@ok
<?php

class Base {
    public function bar() {
        var_dump("Base");
    }
}

class Derived extends Base {
    public function bar() {
        var_dump("Derived");
    }

    public function foo() {
        $f = function() { 
            parent::bar(); 
            $g = function() {
                parent::bar();
                self::bar();
            };
            $g();
        };
        $f();
    }
}


$d = new Derived();
$d->foo();
