@ok
<?php

class Base {
    public static function foo() {
        var_dump("Base");
    }

    public static function bar() {
        $f = function() { static::foo(); };
        $f();

        $f2 = function() {
            $g2 = function() { static::foo(); };
            $g2();

            (function() {
                (function() {
                    (fn() => static::foo())();
                })();
            })();
        };
        $f2();
    }
}

class Derived extends Base {
    public static function foo() {
        var_dump("Derived");
    }
}

Derived::bar();
