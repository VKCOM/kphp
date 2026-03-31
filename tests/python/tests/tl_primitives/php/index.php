<?php

$operations = json_decode(file_get_contents('php://input'));

foreach ($operations as $op) {
  try {
    switch ($op['kind']) {
      case 'rpc_parse':
        if (rpc_parse($op['new_rpc_data']) != $op['expected']) {
          critical_error('error');
        }
        break;
      case 'rpc_clean':
        if (rpc_clean() != $op['expected']) {
          critical_error('error');
        }
        break;
      case 'fetch_int':
        if (fetch_int() != $op['expected']) {
          critical_error('error');
        }
        break;
      case 'fetch_long':
        if (fetch_long() != $op['expected']) {
          critical_error('error');
        }
        break;
      case 'fetch_double':
        if (fetch_double() != $op['expected']) {
          critical_error('error');
        }
        break;
      case 'fetch_string':
        if (fetch_string() != $op['expected']) {
          critical_error('error');
        }
        break;
      default:
        critical_error('unknown op ' . $op['kind']);
    }
  } catch (Exception $e) {
    if (!$op['expected_exception']) {
      throw $e;
    }
  }
}

echo 'ok';
