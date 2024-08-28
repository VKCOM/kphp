@ok k2_skip
<?php

function test_system_call() {
  $status = 0;
  $last_line = system('');
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('', $status);
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo"');
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo"', $status);
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo "', $status);
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo\t"', $status);
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo\n"', $status);
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo\n "', $status);
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo\n\t"', $status);
  echo "Returned with status: $status, last_line: $last_line";

  $last_line = system('echo "foo \nqux bar  \nquz foo \n  baz foo qux "', $status);
  echo "Returned with status: $status, last_line: ";
  var_dump($last_line);
}

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

function test_passthru_call() {
  $status = 0;
  $result = passthru('');
  echo "Returned with status: $status, result: $result";

  $result = passthru('', $status);
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo"');
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo"', $status);
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo "', $status);
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo\t"', $status);
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo\n"', $status);
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo\n "', $status);
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo\n\t"', $status);
  echo "Returned with status: $status, result: $result";

  $result = passthru('echo "foo \nqux bar  \nquz foo \n  baz foo qux "', $status);
  echo "Returned with status: $status, result: ";
  var_dump($result);
}

function test_system_calls_errors() {
  $status = 0;
  $last_line = system('mv /unknown/file/here', $status);
  echo "Returned with status: $status, last_line: ";
  var_dump($last_line);

  $last_line = system('/call/to/unknown/executable', $status);
  echo "Returned with status: $status, last_line: ";
  var_dump($last_line);

  $last_line = system('', $status);
  echo "Returned with status: $status, last_line: ";
  var_dump($last_line);
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

function test_system_passthru_errors() {
  $status = 0;
  $result = passthru('mv /unknown/file/here', $status);
  echo "Returned with status: $status, result: ";
  var_dump($result);

  $result = passthru('/call/to/unknown/executable', $status);
  echo "Returned with status: $status, result: ";
  var_dump($result);

  $result = passthru('', $status);
  echo "Returned with status: $status, result: ";
  var_dump($result);
}

// test_system_call();
// test_system_calls_errors();
test_exec_call();
test_exec_preserve_input();
test_passthru_call();
test_exec_calls_errors();
test_system_passthru_errors();
