@ok
<?php

class Base {
    public static $x = "A";
}

class Derived1 extends Base {}

class Derived2 extends Base {}


Derived1::$x = "A";
Derived2::$x = "B";

var_dump(Derived1::$x, Derived2::$x);

