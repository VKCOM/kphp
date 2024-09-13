<?php

function process_work($arg) {
    $arr = [];
    $arr[] = $arg;
    $arr[] = $arg;
}

$query = component_server_accept_query();
$str = component_server_fetch_request($query);
process_work($str);
process_work('123213');
component_server_send_response($query, $str);
