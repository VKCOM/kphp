<?php

require_once 'request_handlers.php';

$response = handle_request(rpc_server_fetch_request());

if ($response === null) {
  store_error(1, 'server error');
}

rpc_server_store_response($response);
