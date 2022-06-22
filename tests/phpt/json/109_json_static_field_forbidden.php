@kphp_should_fail
/public static \$ids = \[\];/
/@kphp-json is allowed only for instance fields/
<?php

class A {
    /** @kphp-json skip */
    public static $ids = [];
}
