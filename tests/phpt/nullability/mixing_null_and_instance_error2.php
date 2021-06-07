@kphp_should_fail
/assign null to \$v/
/declared as @var Foo/
<?php

class Foo {}

function test() {
  /** @var Foo */
  $v = null;
}

test();

