@ok
<?php

function test_connection($address, $request) {
    $fp = stream_socket_client($address, $errno, $errstr);
    if (!$fp) {
        echo "Failed to create connection<br />\n";
    } else {
        fwrite($fp, $request);
        $response = fread($fp, 2048);
        $statusPos = strpos($response, "\r\n");
        echo substr($response, 0, $statusPos) . "\n";
        fclose($fp);
    }
}

function test_getter($address, $request) {
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

function test_eof($address, $request) {
    $fp = stream_socket_client($address, $errno, $errstr, 30);
    if (!$fp) {
        echo "$errstr ($errno)<br />\n";
    } else {
        fwrite($fp, $request);
        echo fgets($fp, 1024);
        while (!feof($fp)) {
            fgets($fp, 1024);
        }
        fclose($fp);
    }
}

test_connection("tcp://bad_address", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_connection("tcp://wrong_address:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_connection("tcp://example.com:wrong_port", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_connection("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_connection("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: not_working.com:1111\r\nAccept: */*\r\n\r\n");

test_getter("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
test_eof("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: not_working.com:80\r\nAccept: */*\r\n\r\n");
