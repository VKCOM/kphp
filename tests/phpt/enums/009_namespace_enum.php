@ok php8
<?php

include "enums/MyBool.php";

use enums\MyBoolNS\MyBool as AnotherBool;

echo AnotherBool::T->name;
echo AnotherBool::F->name;
