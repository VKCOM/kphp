@kphp_should_fail
<?php

interface WithDefault {
    public function foo($x = 10);
}
