@kphp_should_fail
/Can not fetch instance of mutable class X with instance_cache_fetch call/
<?php

class X {
  public $x = 1;
}

instance_cache_fetch(X::class, "key");
