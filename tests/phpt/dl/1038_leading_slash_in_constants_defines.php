@ok
<?php

define('foo', 23);
var_dump(foo);
var_dump(\foo);
var_dump(defined('foo'));
var_dump(defined('\foo'));

\define('bar', 47);
var_dump(bar);
var_dump(\bar);
var_dump(\defined('bar'));
var_dump(\defined('\bar'));
