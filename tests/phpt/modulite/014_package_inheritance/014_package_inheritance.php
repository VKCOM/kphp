@ok
KPHP_COMPOSER_ROOT={dir}
<?php
#ifndef KPHP
require_once __DIR__ . '/vendor/autoload.php';
require_once 'kphp_tester_include.php';
#endif

\Test\TestClass::test();
