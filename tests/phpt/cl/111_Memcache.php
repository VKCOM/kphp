@ok
<?php
require_once 'Classes/autoload.php';

// it must just compile

function connect(Memcache $mc) {
  $mc->connect('127.0.0.1');
}

if (0) {
  $mc = new Memcache();
  connect($mc);
  $mc->add('12', '34');
  $a = $mc->get('12');
}
