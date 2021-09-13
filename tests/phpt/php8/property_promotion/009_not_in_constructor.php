@kphp_should_fail php8
/Cannot declare promoted property outside a constructor/
<?php

class Test {
     public function foobar(public int $x, public int $y) {}
}
