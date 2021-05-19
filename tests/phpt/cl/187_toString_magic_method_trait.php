@ok
<?php

trait StringableTrait {
    public function __toString(): string {
        return "Foo class";
    }
}

class Foo {
    use StringableTrait;
}

function test() {
    $f = new Foo();
    echo $f;
}

test();
