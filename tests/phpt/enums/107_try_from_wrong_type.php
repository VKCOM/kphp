@kphp_should_fail
/pass A to argument \$value of Status::tryFrom/
/but it's declared as @param mixed/
<?php

enum Status : int {
}

class A{};

$v = Status::tryFrom(new A());
$v = Status::tryFrom(0);
