@kphp_should_fail
/@kphp-json 'fields' specifies 'name', but such field doesn't exist in class User1/
<?php

/**
 * @kphp-json fields = id, name
 */
class User1 {
    public int $id = 0;
}
