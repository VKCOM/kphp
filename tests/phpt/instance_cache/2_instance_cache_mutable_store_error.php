@kphp_should_fail k2_skip
/Can not store instance of mutable class X with instance_cache_store call/
<?php

class X {
  public $x = 1;
}

instance_cache_store("key", new X);
