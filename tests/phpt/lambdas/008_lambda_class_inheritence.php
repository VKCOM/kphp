@ok
<?php

class A {
    const VALUE = 1;
    static public $field = 10;

    static function hasLambda() {
        (function() {
            echo "getGroupId static=", static::class, " value=", static::VALUE, "\n";
            echo "getGroupId self=", self::class, " value=", self::VALUE, "\n";
            self::buildObject();
            static::buildObject();
        })();
    }

    static function getField(): string {
        $cb = fn() => static::$field . ' ' . self::$field;
        return $cb();
    }

    static function buildObject() {
        echo "A buildObject ", static::class, "\n";
        self::$field += 2;
    }
}

class B extends A {
    const VALUE = 2;

    static function buildObject() {
        echo "B buildObject ", static::class, "\n";
        self::$field += 10;
    }
}

class C extends B {
    const VALUE = 3;
    static public $field = 100;

    static function buildObject() {
        echo "C buildObject ", static::class, "\n";
    }
}

A::hasLambda();
B::hasLambda();
C::hasLambda();
echo A::$field, ' ', B::$field, ' ', C::$field, "\n";
echo A::getField(), ' ', B::getField(), ' ', C::getField(), "\n";
