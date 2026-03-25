@ok tcp_server:48093
<?php

function test_no_close($address, $request) {
    $fp = stream_socket_client($address, $errno, $errstr);
}


test_no_close("tcp://127.0.0.1:48091", "GET / HTTP/1.0\r\nHost: 127.0.0.1:48091\r\nAccept: */*\r\n\r\n");
