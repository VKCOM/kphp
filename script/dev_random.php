<?php

$stream_to_out = component_open_stream("out");
if (is_null($stream_to_out)) {
    warning("cannot open stream to out");
    exit(1);
}


while(!$stream_to_out->is_write_closed() && !$stream_to_out->is_please_shutdown_write()) {
    component_stream_write_nonblock($stream_to_out, "random_batch");
}

// while (true) {
//     component_server_accept_query();
//     component_server_set_please_shutdown();
//     while (!component_server_is_write_closed() && !component_server_is_please_shutdown()) {
//         component_server_write_nonblock("random_batch");
//     }
// }
