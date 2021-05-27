@ok
<?php

class StringableClass {
    public function __toString(): string {
        return "StringableClass class";
    }
}

class Goo extends StringableClass {}
class Foo extends Goo {
    public function __toString(): string {
        return "Foo class";
    }
}

function test() {
    $f = new Foo();
    echo $f;
}

test();
