<?php

function process_work($arg) {
    $arr = [];
    $arr[] = $arg;
    $arr[] = $arg;
}

function is_component_query() {
    return $_SERVER['QUERY_TYPE'] === 'component';
}

if (is_component_query()) {
    $str = component_server_get_query();
    process_work($str);
    process_work('123213');
    component_server_send_result($str);
}
