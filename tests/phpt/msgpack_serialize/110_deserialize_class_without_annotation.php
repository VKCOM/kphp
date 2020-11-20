@kphp_should_fail
/You may not deserialize class without @kphp-serializable tag A/
<?php

class A { public $x = 1; };
instance_deserialize("", A::class);
