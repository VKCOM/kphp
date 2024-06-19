<?php

while (true) {
    $query = component_server_get_query();
    warning("got query " . $query);
    $res = "unsupported \"" . $query . "\"";
    component_server_send_result($query);
    warning("send result " . $res);
}
