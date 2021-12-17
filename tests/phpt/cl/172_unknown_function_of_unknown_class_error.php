@kphp_should_fail
/Invalid call \.\.\.->foo\(\): UnknownClass\$\$create\(\) does not return an instance or it can't be detected/
<?php

function test() {
   $x = UnknownClass::create()->foo();
   var_dump($x);
}

test();
