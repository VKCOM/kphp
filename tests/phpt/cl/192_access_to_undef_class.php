@kphp_should_fail
/class Foo not found/
<?php

function test_access_to_undef_class() {
  var_dump(Foo::$static_var_x);
}

test_access_to_undef_class();
