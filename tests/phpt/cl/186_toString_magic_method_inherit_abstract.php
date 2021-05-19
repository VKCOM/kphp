@ok
<?php

abstract class AbstractStringable {
    public function __toString(): string {
        return "Foo class";
    }
}

class Foo extends AbstractStringable {}

function test() {
    $f = new Foo();
    echo $f;
}

test();
