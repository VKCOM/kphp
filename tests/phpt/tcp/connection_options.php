@ok
<?php

function test_non_blocking_connection($address, $request) {
    $fp = stream_socket_client($address, $errno, $errstr);
    if (!$fp) {
        echo "Failed to create connection<br />\n";
    } else {
        stream_set_blocking($fp, false);
        for ($writed = 0; $writed < strlen($request);) {
            $writed = $writed + fwrite($fp, substr($request, $writed));
        }
        $response = fread($fp, 2048);
        if (!$response) {
            echo "Response not ready yet";
        }
        while (!$response) {
            $response = fread($fp, 2048);
        }
        $statusPos = strpos($response, "\r\n");
        echo substr($response, 0, $statusPos);
        fclose($fp);
    }
}

test_non_blocking_connection("tcp://example.com:80", "GET / HTTP/1.0\r\nHost: example.com:80\r\nAccept: */*\r\n\r\n");
