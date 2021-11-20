@kphp_should_fail php8
/Redeclaration of Test::\$prop/
<?php

class Test {
     public $prop;

     public function __construct(public $prop) {}
}
