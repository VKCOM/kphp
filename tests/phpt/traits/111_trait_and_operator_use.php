@kphp_should_fail
/Usage of operator `use`\(Aliasing/Importing\) with traits is temporarily prohibited/
<?php

require_once 'polyfills.php';
use Classes2\Classes2A;

trait SomeTrait {}
