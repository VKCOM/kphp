@kphp_should_fail
/@kphp-json for MyJsonEncoder 'skip_if_default' should be placed below @kphp-json 'skip_if_default' without for/
<?php

class MyJsonEncoder extends JsonEncoder {}

/**
 * @kphp-json for MyJsonEncoder skip_if_default = false
 * @kphp-json skip_if_default
 */
class A {
    public int $id = 0;
}

