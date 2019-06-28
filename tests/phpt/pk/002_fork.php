@ok
<?php
require_once __DIR__."/../dl/polyfill/fork-php-polyfill.php";

function add_res($value) {
  global $res;
  sched_yield();
  $res += $value;
}
function hash2($n) {
  global $res;
  for ($i = 0; $i < $n; $i++) {
    $hash1["foo_".rand()] = 1;
  }
  $ids = array();
  for ($i = $n; $i > 0; $i--) {
    foreach($hash1 as $value) {
      $ids[] = fork(add_res($value));
    }
  }
  foreach ($ids as $id){
    wait($id);
  }
}
function hash3($n) {
  global $res;
  for ($i = 0; $i < $n; $i++) {
    $hash1["foo_".rand()] = 1;
  }
  $ids = array();
  for ($i = $n; $i > 0; $i--) {
    foreach($hash1 as $value) {
      $ids[] = fork(add_res($value));
    }
  }
  $q = wait_queue_create($ids);
  while (true) {
    $t = wait_queue_next ($q);
    if (!$t) {
      break;
    }
    wait_result ($t);
  }
}

function hash4($n) {
  global $res;
  for ($i = 0; $i < $n; $i++) {
    $hash1["foo_".rand()] = 1;
  }
  $q = wait_queue_create([]);
  for ($i = $n; $i > 0; $i--) {
    foreach($hash1 as $key=>$value) {
      switch ((int) $key % 3) {
        case 0:
          $id = fork(add_res($value));
      wait_queue_push($q, $id);
          break;
        case 1:
          $id = fork(add_res($value));
      wait_queue_push($q, $id);
          break;
        case 2:
          $id = fork(add_res($value));
      wait_queue_push($q, $id);
          break;
        case "iamstring!":
          break;
      }
    }
  }
  return $q;
}



$id2 = fork(hash2(100));
$id = fork(hash3(100));
wait_result($id);
wait_result($id2);

print "{$res}\n";

# $id3 = fork(hash4(100));
$qid = hash4(100);
while (true) {
  $t = wait_queue_next ($qid);
  if (!$t) {
    break;
  }
  wait_result ($t);
}

print "{$res}\n";
