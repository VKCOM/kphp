@ok k2_skip
<?php

function test_escapeshellarg() {
  echo escapeshellarg(""), "\n";
  echo escapeshellarg("'"), "\n";
  echo escapeshellarg("''"), "\n";
  echo escapeshellarg("ls bar"), "\n";
  echo escapeshellarg("ls 'foo'"), "\n";
}

function test_escapeshellcmd_quotes() {
  echo escapeshellcmd(""), "\n";
  echo escapeshellcmd("foo"), "\n";
  echo escapeshellcmd("'"), "\n";
  echo escapeshellcmd("''"), "\n";
  echo escapeshellcmd("'''"), "\n";
  echo escapeshellcmd("'foo"), "\n";
  echo escapeshellcmd("''foo"), "\n";
  echo escapeshellcmd("foo'"), "\n";
  echo escapeshellcmd("foo''"), "\n";
  echo escapeshellcmd("'foo'"), "\n";
  echo escapeshellcmd("'foo'bar"), "\n";
  echo escapeshellcmd("''foo'bar'"), "\n";
  echo escapeshellcmd("'\""), "\n";
  echo escapeshellcmd("'\"'"), "\n";
  echo escapeshellcmd("'foo\"'"), "\n";
  echo escapeshellcmd("'\"foo'"), "\n";
  echo escapeshellcmd("'foo\"foo'"), "\n";
  echo escapeshellcmd("'\"\"foo"), "\n";
}

function test_escapeshellcmd() {
  echo escapeshellcmd('#'), "\n";
  echo escapeshellcmd('&'), "\n";
  echo escapeshellcmd(';'), "\n";
  echo escapeshellcmd('`'), "\n";
  echo escapeshellcmd('|'), "\n";
  echo escapeshellcmd('*'), "\n";
  echo escapeshellcmd('?'), "\n";
  echo escapeshellcmd('~'), "\n";
  echo escapeshellcmd('<'), "\n";
  echo escapeshellcmd('>'), "\n";
  echo escapeshellcmd('^'), "\n";
  echo escapeshellcmd('('), "\n";
  echo escapeshellcmd(')'), "\n";
  echo escapeshellcmd('['), "\n";
  echo escapeshellcmd(']'), "\n";
  echo escapeshellcmd('{'), "\n";
  echo escapeshellcmd('}'), "\n";
  echo escapeshellcmd('$'), "\n";
  echo escapeshellcmd('\\'), "\n";
  echo escapeshellcmd('\x0A'), "\n";
  echo escapeshellcmd('\xFF'), "\n";

  echo escapeshellcmd('foo'), "\n";
  echo escapeshellcmd('#foo'), "\n";
  echo escapeshellcmd('foo#'), "\n";
  echo escapeshellcmd('#foo#'), "\n";
  echo escapeshellcmd('foo##'), "\n";
}

test_escapeshellarg();
test_escapeshellcmd_quotes();
test_escapeshellcmd();
