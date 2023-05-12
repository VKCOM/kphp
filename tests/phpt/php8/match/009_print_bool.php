@ok php8
<?php

function print_bool($bool) {
  echo match ($bool) {
    true => "true\n",
    false => "false\n"
  };
}

print_bool(true);
print_bool(false);
