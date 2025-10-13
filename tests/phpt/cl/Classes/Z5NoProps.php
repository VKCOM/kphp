<?php

namespace Classes;

class Z5NoProps {
    public function sayHello() {
        echo "z5 hello\n";
    }

    public static function sayHelloStatic() {
        (new self())->sayHello();
    }
}
