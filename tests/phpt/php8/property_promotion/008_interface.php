@kphp_should_fail php8
/Cannot declare promoted property in an abstract constructor/
<?php

interface Test {
     public function __construct(public int $x);
}
