@kphp_should_fail
/Called instance_deserialize\(\) for class A, but it's not marked with @kphp-serializable/
<?php

class A { public $x = 1; };
instance_deserialize("", A::class);
