<?php

$query = component_server_accept_query();
$str = component_server_fetch_request($query);
$id = component_client_send_request("out", $str);
if (is_null($id)) {
    warning("stream to out is null");
} else {
    $result = component_client_fetch_response($id);
    component_server_send_response($query, $result);
}
