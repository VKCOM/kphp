@kphp_should_fail
/Magic method Goo::__toString must have string return type/
/Magic method Foo::__toString cannot take arguments/
<?php
class Foo {
    public function __toString($a): string {
        return $a;
    }
}

class Goo {
    public function __toString(): int {
        return 12;
    }
}
