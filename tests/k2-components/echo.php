<?php

function process_work($arg) {
    $arr = [];
    $arr[] = $arg;
    $arr[] = $arg;
}

$str = component_server_get_query();
process_work($str);
process_work('123213');
component_server_send_result($str);
