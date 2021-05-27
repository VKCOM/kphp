@ok
<?php

interface Stringable {
    public function __toString(): string;
}

class Foo implements Stringable {
    public function __toString(): string {
        return "Foo class";
    }
}

function test() {
    $f = new Foo();
    echo $f;
}

test();
