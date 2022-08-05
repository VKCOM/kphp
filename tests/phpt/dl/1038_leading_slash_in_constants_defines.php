@ok
<?php

define('foo', 23);
var_dump(foo);
var_dump(\foo);
var_dump(defined('foo'));
var_dump(defined('\foo'));
