<?php

$operations = json_decode(file_get_contents('php://input'));

foreach ($operations as $op) {
  try {
    switch ($op['kind']) {
      case 'rpc_parse':
        $res = rpc_parse($op['new_rpc_data']);
        if ($res != $op['expected']) {
          critical_error('expected ' . $op['expected'] . ' but ' . $res . '  found');
        }
        break;
      case 'rpc_clean':
        $res = rpc_clean();
        if ($res != $op['expected']) {
          critical_error('expected ' . $op['expected'] . ' but ' . $res . '  found');
        }
        break;
      case 'fetch_int':
        $res = fetch_int();
        if ($res != $op['expected']) {
          critical_error('expected ' . $op['expected'] . ' but ' . $res . '  found');
        }
        break;
      case 'fetch_long':
        $res = fetch_long();
        if ($res != $op['expected']) {
          critical_error('expected ' . $op['expected'] . ' but ' . $res . '  found');
        }
        break;
      case 'fetch_double':
        $res = fetch_double();
        if ($res != $op['expected']) {
          critical_error('expected ' . $op['expected'] . ' but ' . $res . '  found');
        }
        break;
      case 'fetch_string':
        $res = fetch_string();
        if ($res != $op['expected']) {
          critical_error('expected ' . $op['expected'] . ' but ' . $res . '  found');
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
