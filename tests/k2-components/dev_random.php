<?php

$input = component_accept_stream();

$stream_to_out = component_open_stream("out");
if (is_null($stream_to_out)) {
    warning("cannot open stream to out");
    exit(1);
}


while(!$stream_to_out->is_write_closed() && !$stream_to_out->is_please_shutdown_write()) {
    component_stream_write_exact($stream_to_out, "a");
}

component_close_stream($stream_to_out);
component_finish_stream_processing($input);

$input = component_accept_stream();

while(!$input->is_write_closed() && !$input->is_please_shutdown_write()) {
    component_stream_write_exact($input, "a");
}

