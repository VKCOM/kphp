@ok
<?php

class Base {
    /**@kphp-infer*/
    public static function foo() {}

    /**
     * @kphp-infer
     * @return null
     */
    public static function foo_null() { return null; }

    /**@kphp-infer*/
    public static function void_foo() {}
}

class Derived extends Base {
    /**@kphp-infer*/
    public static function foo() {}
}

Derived::foo();
Derived::foo_null();
Derived::void_foo();
var_dump("OK");
