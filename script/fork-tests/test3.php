<?php

function call($message) {
    warning($message );
    $query = component_client_send_query("KPHP-server", $message);
    $result = component_client_get_result($query);
    warning("server return - " . $result);
    return $result;
}

$forks_count = 3;
$futures = [];
$message_prefix = "hello from fork ";
for ($i = 0; $i < $forks_count; ++$i) {
    $fork_message = $message_prefix . ($i + 1);
    $futures[] = fork(call($fork_message));
}

for ($i = 0; $i < $forks_count; ++$i) {
    $result = wait($futures[$i]);
}



