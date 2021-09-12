@ok php8
<?php

function print_this() {
  echo "Executed\n";
  return 0;
}

match (1) {
  1 => print_this(),
};
