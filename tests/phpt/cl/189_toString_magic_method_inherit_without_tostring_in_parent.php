@ok
<?php

class Goo {}
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
