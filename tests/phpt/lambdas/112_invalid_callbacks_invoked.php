@kphp_should_fail k2_skip
/Invalid \(\)-invocation of a non-lambda/
/Invalid callback passed, could not detect a class or its method/
<?php

class A{}

$a1 = new A;
$a1();

array_map([$a1, 'unexisting'], [1,2,3]);

