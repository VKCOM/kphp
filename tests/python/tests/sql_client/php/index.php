<?php

/**
 * @param mixed $context
 * @return PDO
 */
function create_connection($context): PDO {
  $mysql_dbname = $context['mysql-dbname'];
  $mysql_host = $context['mysql-host'];
  $mysql_port = $context['mysql-port'];
  $mysql_user = (string)$context['mysql-user'];

  return new PDO("mysql:host=$mysql_host;dbname=$mysql_dbname;port=$mysql_port", (string)$mysql_user);
}

function mysql_test_query() {
  $context = json_decode(file_get_contents('php://input'));

  $db = create_connection($context);
  $query_text = (string)$context['query'];
  if (substr($query_text, 0, 6) === 'SELECT') {
    $stmt = $db->query($query_text);
    if ($db->errorCode() == 0) {
      $res = $stmt->fetchAll();
    } else {
      $res = ['error' => array_slice($db->errorInfo(), 0, 2)];
    }
  } else {
    $affected_rows = $db->exec($query_text);
    if ($db->errorCode() == 0) {
      $res = ['affected_rows' => $affected_rows];
    } else {
      $res = ['error' => array_slice($db->errorInfo(), 0, 2)];
    }
  }
  echo json_encode(['result' => $res]);
}

function main() {
    switch($_SERVER["PHP_SELF"]) {
        case "/mysql_test_query": {
          mysql_test_query();
          return;
        }
        default: {
          critical_error("Unknown test " . $_SERVER["PHP_SELF"]);
        }
    }
}

main();
