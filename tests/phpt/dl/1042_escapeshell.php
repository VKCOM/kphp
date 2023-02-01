@ok
<?php

function test_escapeshellarg() {
  echo escapeshellarg(""), "\n";
  echo escapeshellarg("'"), "\n";
  echo escapeshellarg("''"), "\n";
  echo escapeshellarg("ls bar"), "\n";
  echo escapeshellarg("ls 'foo'"), "\n";
}

test_escapeshellarg();
