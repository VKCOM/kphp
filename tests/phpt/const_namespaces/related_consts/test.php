@ok
<?php

require_once __DIR__ . '/file2.php';

use const Example\C1;
use Foo\Bar;

var_dump([
  C1,
  \Example\C1,
  \Example\C2,
  \Example\C3,
  \Example\C4,
  Bar\Baz,
]);

var_dump([
  // defined() ignores const uses
  defined('C1'),
  defined('Bar\Baz'),

  defined('Example\C1'),
  defined('Example\C2'),
  defined('Example\C3'),
  defined('Example\C4'),

  defined('\Example\C1'),
  defined('\Example\C2'),
  defined('\Example\C3'),
  defined('\Example\C4'),
]);
