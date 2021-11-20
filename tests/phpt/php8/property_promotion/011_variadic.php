@kphp_should_fail php8
/Cannot declare variadic promoted property/
<?php

class Test {
     public function __construct(public string ...$strings) {}
}
