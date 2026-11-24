@ok
<?php

require_once 'kphp_tester_include.php';

class First
{
  /** @var Second */
  public $log;

  public string $value = '';
}

class Second
{
  /** @var First[] */
  public $rows = [];
}

$log = new Second();
$row = new First();
$row->value = 'cycle';
$row->log = $log;
$log->rows[] = $row;

if (kphp) {
  $dump = to_array_debug($log, true);
} else {
  // The PHP polyfill does not handle recursive object graphs.
  $dump = [
    'rows' => [[
      'log' => [],
      'value' => 'cycle',
      '__class_name' => 'First',
    ]],
    '__class_name' => 'Second',
  ];
}

var_dump($dump);
