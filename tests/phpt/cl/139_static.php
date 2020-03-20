@ok
<?php
require_once 'polyfills.php';

Classes\G::static_public_func();
Classes\G::public_static_func();

var_dump(Classes\G::$static_public);
var_dump(Classes\G::$public_static);
