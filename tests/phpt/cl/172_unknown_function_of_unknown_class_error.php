@kphp_should_fail
/Unknown function ->foo\(\)/
<?php

function test() {
   $x = UnknownClass::create()->foo();
   var_dump($x);
}

test();