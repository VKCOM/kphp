@ok
<?php

function test_exec_call() {
  $status = 0;
  $output = [];

  $last_line = exec('');
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = exec('', $output, $status);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo"');
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = exec('echo "foo"', $output);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo"', $output, $status);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo "', $output, $status);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo\t"', $output, $status);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo\n"', $output, $status);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo\n "', $output, $status);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo\n\t"', $output, $status);
  echo "Returned with status: $status, last_line: $last_line, output:\n";
  var_dump($output);

  $last_line = exec('echo "foo \nqux bar  \nquz foo \n  baz foo qux "', $output, $status);
  echo "Returned with status: $status, output:\n";
  var_dump($output);
  var_dump($last_line);
}

function test_exec_preserve_input() {
  $output = ['old' => 'params'];
  exec('echo "new\nparams"', $output);
  var_dump($output);
}

function test_exec_calls_errors() {
  $status = 0;
  $output = [];

  $last_line = exec('mv /unknown/file/here', $output, $status);
  echo "Returned with status: $status, output:\n";
  var_dump($output);
  var_dump($last_line);

  $last_line = exec('/call/to/unknown/executable', $output, $status);
  echo "Returned with status: $status, output:\n";
  var_dump($output);
  var_dump($last_line);

  $last_line = exec('', $output, $status);
  echo "Returned with status: $status, output:\n";
  var_dump($output);
  var_dump($last_line);
}

test_exec_call();
test_exec_preserve_input();
test_exec_calls_errors();
