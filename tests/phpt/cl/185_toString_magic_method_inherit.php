@ok
<?php

class StringableClass {
    public function __toString(): string {
        return "StringableClass class";
    }
}

class Foo extends StringableClass {}

function test() {
    $f = new Foo();
    echo $f;
}

test();
