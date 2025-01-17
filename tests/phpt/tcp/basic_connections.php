@
<?php

function test_simple_connection($address, $request) {
    $fp = stream_socket_client($address, $errno, $errstr);
    if (!$fp) {
        echo "Failed to create connection<br />\n";
    } else {
        fwrite($fp, $request);
        $responseStatus = fgets($fp, 2048);
        echo $responseStatus;
        $char = fgetc($fp);
        echo $char . "\n";
        fclose($fp);
    }
}

function test_full_response($address, $request) {
    $fp = stream_socket_client($address, $errno, $errstr);
    if (!$fp) {
        echo "Failed to create connection<br />\n";
    } else {
        fwrite($fp, $request);
        $responseStatus = fgets($fp, 2048);
        echo $responseStatus;
        while (!feof($fp)) {
            fgets($fp, 512);
            fread($fp, 512);
        }
        echo feof($fp);
        fclose($fp);
    }
}

function test_no_close($address, $request) {
    $fp = stream_socket_client($address, $errno, $errstr);
}


test_simple_connection("tcp://bad_address", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_simple_connection("tcp://wrong_address:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_simple_connection("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_simple_connection("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: not_working.com:1111\r\nAccept: */*\r\n\r\n");

test_full_response("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_full_response("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: not_working.com:80\r\nAccept: */*\r\n\r\n");

test_no_close("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
