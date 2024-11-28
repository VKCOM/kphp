@ok
<?php
require_once 'kphp_tester_include.php';

class User {
  public string $name = '';
  public string $password = '';
}

function test_optimized_init_value(): void {
  $raw = '{"name": "user"}';

  $user = JsonEncoder::decode($raw, User::class);

  $error = JsonEncoder::getLastError();
  if (!empty($error)) {
    echo "Error: " . $error;
    exit();
  }

  var_dump(instance_to_array($user));
}

test_optimized_init_value();
