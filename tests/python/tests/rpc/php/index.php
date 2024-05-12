<?php


require_once "rpc_extra_info.php";
require_once "rpc_wrappers.php";

function do_http_worker() {
  switch($_SERVER["PHP_SELF"]) {
    case "/test_kphp_untyped_rpc_extra_info": {
      test_kphp_untyped_rpc_extra_info();
      return;
    }
    case "/test_kphp_typed_rpc_extra_info": {
      test_kphp_typed_rpc_extra_info();
      return;
    }
    case "/test_rpc_no_wrappers_with_actor_id": {
      test_rpc_no_wrappers_with_actor_id();
      return;
    }
    case "/test_rpc_no_wrappers_with_ignore_answer": {
      test_rpc_no_wrappers_with_ignore_answer();
      return;
    }
    case "/test_rpc_dest_actor_with_actor_id": {
      test_rpc_dest_actor_with_actor_id();
      return;
    }
    case "/test_rpc_dest_actor_with_ignore_answer": {
      test_rpc_dest_actor_with_ignore_answer();
      return;
    }
    case "/test_rpc_dest_flags_with_actor_id": {
      test_rpc_dest_flags_with_actor_id();
      return;
    }
    case "/test_rpc_dest_flags_with_ignore_answer": {
      test_rpc_dest_flags_with_ignore_answer();
      return;
    }
    case "/test_rpc_dest_flags_with_ignore_answer_1": {
      test_rpc_dest_flags_with_ignore_answer_1();
      return;
    }
    case "/test_rpc_dest_actor_flags_with_actor_id": {
      test_rpc_dest_actor_flags_with_actor_id();
      return;
    }
    case "/test_rpc_dest_actor_flags_with_ignore_answer": {
      test_rpc_dest_actor_flags_with_ignore_answer();
      return;
    }
    case "/test_rpc_dest_actor_flags_with_ignore_answer_1": {
      test_rpc_dest_actor_flags_with_ignore_answer_1();
      return;
    }
  }

  critical_error("unreachable code!");
}

do_http_worker();
