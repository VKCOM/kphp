@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

// it must just compile

function connect(Memcache $mc) {
  $mc->addServer('127.0.0.1');
}

if (0) {
  $mc = new Memcache();
  connect($mc);
  $mc->add('12', '34');
  $a = $mc->get('12');
}
