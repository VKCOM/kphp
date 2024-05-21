<?php

$str = component_server_get_query();
$id = component_client_send_query("out", $str);
if (is_null($id)) {
    warning("stream to out is null");
} else {
    $result = component_client_get_result($id);
    component_server_send_result($result);
}
