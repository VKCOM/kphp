@kphp_should_fail
/Error compiling Base::foo, because Base has no inheritors/
/Provide types for all params/
<?php

abstract class Base {
    abstract public function foo($arg);
    public static function use_base() {
        var_dump("use_base");
    }
}

Base::use_base();

