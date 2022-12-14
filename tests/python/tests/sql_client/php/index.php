<?php

/**
 * @param string $name
 * @param mixed $context
 * @return PDO
 */
function create_connection($name, $context): PDO {
  $dbname = $context['dbname'];
  $host = $context['host'];
  $port = $context['port'];
  $user = (string)$context['user'];

  return new PDO("$name:host=$host;dbname=$dbname;port=$port", (string)$user);
}

function simple_function() {
    fwrite(STDERR, "start_resumable_function\n");
    sched_yield_sleep(0.3);
    fwrite(STDERR, "end_resumable_function\n");
    return true;
}

function db_test_query($name) {
  $context = json_decode(file_get_contents('php://input'));

  $db = create_connection($name, $context);
  $query_text = (string)$context['query'];
  if (substr($query_text, 0, 6) === 'SELECT') {
    fwrite(STDERR, "start_query\n");
    $stmt = $db->query($query_text);
    fwrite(STDERR, "end_query\n");
    if ($db->errorCode() == 0) {
      $res = $stmt->fetchAll();
    } else {
      $res = ['error' => array_slice($db->errorInfo(), 0, 2)];
    }
  } else {
    fwrite(STDERR, "start_exec\n");
    $affected_rows = $db->exec($query_text);
    fwrite(STDERR, "end_exec\n");
    if ($db->errorCode() == 0) {
      $res = ['affected_rows' => $affected_rows];
    } else {
      $res = ['error' => array_slice($db->errorInfo(), 0, 2)];
    }
  }
  echo json_encode(['result' => $res]);
}

function main() {
    $name = (string)$_GET["name"];
    switch($_SERVER["PHP_SELF"]) {
        case "/": {
          db_test_query($name);
          return;
        }
        case "/resumable_test": {
          $future = fork(simple_function());
          db_test_query($name);
          wait($future);
          return;
        }
        default: {
          critical_error("Unknown test " . $_SERVER["PHP_SELF"]);
        }
    }
}

main();
