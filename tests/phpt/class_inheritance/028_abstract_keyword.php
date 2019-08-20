@ok
<?php

abstract class Base {
    abstract public function foo();
    public static function use_base() {
        var_dump("use_base");
    }
}

Base::use_base();

