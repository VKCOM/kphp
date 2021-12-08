@kphp_should_fail php8
/Private constant cannot be final as it is not visible to other classes/
<?php

class Foo {
  private final const A = "foo";
}
