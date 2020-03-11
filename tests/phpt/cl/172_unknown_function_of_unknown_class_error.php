@kphp_should_fail
/Unknown function ->foo\(\) of Unknown class/
<?php

function test() {
   $x = UnknownClass::create()->foo();
   var_dump($x);
}

test();