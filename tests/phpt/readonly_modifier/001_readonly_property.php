@kphp_should_fail
/Modification of const/
<?php

class Foo {
    public readonly string $prop_readonly = "hello";
    public readonly string $prop_default = "hello";

    public function __construct() {
        $this->prop = "ssss"; // ok
    }
}

$foo = new Foo();
$foo->prop_readonly = "world"; // error
$foo->prop_default = "world";  // ok
