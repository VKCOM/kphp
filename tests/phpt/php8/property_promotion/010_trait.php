@ok php8
<?php

trait Test {
     public function __construct(public $prop) {}
}

class Test2 {
    use Test;
}

var_dump((new Test2(42))->prop);
