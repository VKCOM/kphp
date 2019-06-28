<?php

#ifndef KittenPHP

$__forked = array();

function fork($x) {
  global $__forked;

  $__forked[] = $x;
  return count($__forked);
}

function wait($id, $timeout = -1.0) {
  global $__forked;
  return 0 < $id && $id <= count($__forked) && $__forked[$id - 1] !== '__already_gotten__';
}

function wait_result($id, $timeout = -1.0) {
  global $__forked;

  if (!wait($id, $timeout)) {
    return false;
  }

  $result = $__forked[$id - 1];

  $__forked[$id - 1] = '__already_gotten__';

  return $result;
}

function wait_synchronously($id) {
  return wait($id, -1);
}

function wait_multiple($id) {
  static $waiting = array();

  if (!array_key_exists($id, $waiting)) {
    $waiting[$id] = true;
    wait($id);
    unset($waiting[$id]);
  } else {
    while ($waiting[$id]) {
      sched_yield();
      if ($waiting[$id]) {
        usleep(10 * 1000);
      }
    }
  }
}

function wait_queue_create($request_ids) {
  if (is_array($request_ids)) {
    return $request_ids;
  }
  return array($request_ids);
}

function wait_queue_push(&$queue_id, $request_ids) {
  if ($queue_id == -1) {
    return wait_queue_create($request_ids);
  }

  if (is_array($request_ids)) {
    $queue_id += $request_ids;
  } else {
    $queue_id[] = $request_ids;
  }

  return $queue_id;
}

function wait_queue_empty($queue_id) {
  return $queue_id === -1 || empty($queue_id);
}

function wait_queue_next(&$queue_id, $timeout = -1.0) {
  if (wait_queue_empty($queue_id)) {
    return 0;
  }
  return array_pop($queue_id);
}

function wait_queue_next_synchronously($queue_id) {
  return wait_queue_next($queue_id, -1);
}

function sched_yield() {
  return;
}

function rpc_tl_query_result_synchronously($query_ids) {
  return rpc_tl_query_result($query_ids);
}

function get_running_fork_id() {
  return 0;
}

#endif
