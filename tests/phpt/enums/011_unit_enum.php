@ok php8
<?php

include "enums/MyBool.php";

use enums\MyBoolNS\MyBool as AnotherBool;

$t = AnotherBool::T;
echo ($t instanceof UnitEnum);
echo (AnotherBool::T instanceof \UnitEnum);
echo (AnotherBool::F instanceof \UnitEnum);

class Foo {
}
echo (new Foo() instanceof \UnitEnum);
