@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function ret_bool(int $line, bool $res) : bool {
  echo "call ret_bool:$line\n";
  sched_yield();
  return $res;
}

function test_lhs(bool $always_false, bool $always_true) {
  $x = ret_bool(__LINE__, true) || $always_false;
  var_dump($x);

  $x = ret_bool(__LINE__, false) || $always_false;
  var_dump($x);

  $x = ret_bool(__LINE__, true) || $always_true;
  var_dump($x);

  $x = ret_bool(__LINE__, false) || $always_true;
  var_dump($x);

  $x = ret_bool(__LINE__, true) && $always_false;
  var_dump($x);

  $x = ret_bool(__LINE__, false) && $always_false;
  var_dump($x);

  $x = ret_bool(__LINE__, true) && $always_true;
  var_dump($x);

  $x = ret_bool(__LINE__, false) && $always_true;
  var_dump($x);
  return null;
}

function test_lhs_neg(bool $always_false, bool $always_true) {
  $x = !ret_bool(__LINE__, true) || $always_false;
  var_dump($x);

  $x = !ret_bool(__LINE__, false) || $always_false;
  var_dump($x);

  $x = !ret_bool(__LINE__, true) || $always_true;
  var_dump($x);

  $x = !ret_bool(__LINE__, false) || $always_true;
  var_dump($x);

  $x = !ret_bool(__LINE__, true) && $always_false;
  var_dump($x);

  $x = !ret_bool(__LINE__, false) && $always_false;
  var_dump($x);

  $x = !ret_bool(__LINE__, true) && $always_true;
  var_dump($x);

  $x = !ret_bool(__LINE__, false) && $always_true;
  var_dump($x);
  return null;
}


function test_rhs(bool $always_false, bool $always_true) {
  $x = $always_false || ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_false || ret_bool(__LINE__, false);
  var_dump($x);

  $x = $always_true || ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_true || ret_bool(__LINE__, false);
  var_dump($x);

  $x = $always_false && ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_false && ret_bool(__LINE__, false);
  var_dump($x);

  $x = $always_true && ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_true && ret_bool(__LINE__, false);
  var_dump($x);
  return null;
}

function test_rhs_neg(bool $always_false, bool $always_true) {
  $x = $always_false || !ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_false || !ret_bool(__LINE__, false);
  var_dump($x);

  $x = $always_true || !ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_true || !ret_bool(__LINE__, false);
  var_dump($x);

  $x = $always_false && !ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_false && !ret_bool(__LINE__, false);
  var_dump($x);

  $x = $always_true && !ret_bool(__LINE__, true);
  var_dump($x);

  $x = $always_true && !ret_bool(__LINE__, false);
  var_dump($x);
  return null;
}

function test_lhs_rhs() {
  $x = ret_bool(__LINE__, true) ||
       ret_bool(__LINE__, true);
  var_dump($x);

  $x = ret_bool(__LINE__, false) ||
       ret_bool(__LINE__, false);
  var_dump($x);

  $x = ret_bool(__LINE__, false) ||
       ret_bool(__LINE__, true);
  var_dump($x);

  $x = ret_bool(__LINE__, true) ||
       ret_bool(__LINE__, false);
  var_dump($x);

  $x = ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, true);
  var_dump($x);

  $x = ret_bool(__LINE__, false) &&
       ret_bool(__LINE__, false);
  var_dump($x);

  $x = ret_bool(__LINE__, false) &&
       ret_bool(__LINE__, true);
  var_dump($x);

  $x = ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, false);
  var_dump($x);
  return null;
}

function test_lhs_rhs_neg() {
  $x = !ret_bool(__LINE__, true) ||
       !ret_bool(__LINE__, true);
  var_dump($x);

  $x = !ret_bool(__LINE__, false) ||
       !ret_bool(__LINE__, false);
  var_dump($x);

  $x = !ret_bool(__LINE__, false) ||
       !ret_bool(__LINE__, true);
  var_dump($x);

  $x = !ret_bool(__LINE__, true) ||
       !ret_bool(__LINE__, false);
  var_dump($x);

  $x = !ret_bool(__LINE__, true) &&
       !ret_bool(__LINE__, true);
  var_dump($x);

  $x = !ret_bool(__LINE__, false) &&
       !ret_bool(__LINE__, false);
  var_dump($x);

  $x = !ret_bool(__LINE__, false) &&
       !ret_bool(__LINE__, true);
  var_dump($x);

  $x = !ret_bool(__LINE__, true) &&
       !ret_bool(__LINE__, false);
  var_dump($x);
  return null;
}

function test_long_expresion() {
  $x = ret_bool(__LINE__, false) ||
       ret_bool(__LINE__, false) ||
       !ret_bool(__LINE__, true) ||
       ret_bool(__LINE__, false) ||
       ret_bool(__LINE__, true) ||
       ret_bool(__LINE__, false) ||
       ret_bool(__LINE__, false);
  var_dump($x);

  $x = ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, true) &&
       !ret_bool(__LINE__, false) &&
       ret_bool(__LINE__, false) &&
       ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, true);
  var_dump($x);

  $x = ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, true) ||
       ret_bool(__LINE__, false) ||
       !ret_bool(__LINE__, false) ||
       ret_bool(__LINE__, true) &&
       ret_bool(__LINE__, true) ||
       ret_bool(__LINE__, true);
  var_dump($x);
  return null;
}

function test_in_if() {
  if (ret_bool(__LINE__, true) &&
      ret_bool(__LINE__, true) &&
      !ret_bool(__LINE__, false)) {
    echo "if body1\n";
  } else {
    echo "else body1\n";
  }

  if (ret_bool(__LINE__, false) ||
      !ret_bool(__LINE__, true) ||
      ret_bool(__LINE__, true)) {
    echo "if body2\n";
  } else {
    echo "else body2\n";
  }

  if (ret_bool(__LINE__, false) &&
      ret_bool(__LINE__, true) ||
      ret_bool(__LINE__, true)) {
    echo "if body3\n";
  } else if (ret_bool(__LINE__, false) ||
             ret_bool(__LINE__, true)) {
    echo "else if body3\n";
  } else {
    echo "else body3\n";
  }
  return null;
}

wait(fork(test_lhs(false, true)));
wait(fork(test_lhs_neg(false, true)));
wait(fork(test_rhs(false, true)));
wait(fork(test_rhs_neg(false, true)));
wait(fork(test_lhs_rhs()));
wait(fork(test_lhs_rhs_neg()));
wait(fork(test_long_expresion()));
wait(fork(test_in_if()));
