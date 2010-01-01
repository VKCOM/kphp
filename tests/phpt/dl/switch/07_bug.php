@ok
<?php
function f($x) {
  echo $x.": ";
  switch ($x) {
  case 'preload_faces': var_dump('preload_faces'); break;
  case 'preload': var_dump('preload');
  default: var_dump('qq');
  }
  var_dump('blabla');
}

f('preload');
f('preload_faces');
f('blabla');
f('qq');
f('p');
f('');
f('pr');

echo "\n";

function g($x) {
  echo $x.": ";
  switch ($x) {
  case 'preload_faces': var_dump('preload_faces'); return;
  case 'preload': var_dump('preload'); return;
  default: var_dump('qq'); return;
  }
  var_dump('blabla');
}

g('preload');
g('preload_faces');
g('blabla');
g('qq');
g('p');
g('');
g('pr');
