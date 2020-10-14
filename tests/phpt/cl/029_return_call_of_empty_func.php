@ok
<?php

class Base {
    public static function foo() {}

    /**
     * @return null
     */
    public static function foo_null() { return null; }

    public static function void_foo() {}
}

class Derived extends Base {
    public static function foo() {}
}

Derived::foo();
Derived::foo_null();
Derived::void_foo();
var_dump("OK");
