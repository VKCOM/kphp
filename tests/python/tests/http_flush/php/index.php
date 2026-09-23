<?php

function flush_from_fork(string $tag): int {
    $id = (int)get_running_fork_id();
    echo str_repeat($tag, 100000);
    flush();
    if ((int)get_running_fork_id() !== $id) {
        critical_error('flush changed the running fork ID');
    }
    return 1;
}

switch ($_SERVER['PHP_SELF']) {
    case '/no-body':
        $status = (int)$_GET['status'];
        header("HTTP/1.1 $status No Body");
        echo 'first';
        flush();
        usleep(100000);
        for ($i = 0; $i < 4; ++$i) {
            echo str_repeat('x', 65536);
            flush();
        }
        fwrite(fopen('php://stderr', 'w'), "no-body completed $status\n");
        break;
    case '/content-length':
        header('Content-Length: 10');
        echo 'first';
        flush();
        echo '-last';
        break;
    case '/stream':
        echo "first\n";
        flush();
        usleep(500000);
        echo "last\n";
        break;
    case '/headers':
        echo (int)headers_sent();
        header('X-Before: yes');
        flush();
        echo (int)headers_sent();
        header('X-After: no');
        echo (int)header_register_callback(function () {
            echo 'unexpected callback';
        });
        break;
    case '/callback-final':
    case '/callback-flush':
        header_register_callback(function () {
            echo 'callback';
            flush();
            usleep(10000);
            echo '-after';
        });
        echo 'before-';
        if ($_SERVER['PHP_SELF'] === '/callback-flush') {
            flush();
        }
        break;
    case '/gzip':
    case '/gzip-close':
        ob_start('ob_gzhandler');
        echo 'first';
        ob_flush();
        flush();
        flush(); // No input after a previous Z_SYNC_FLUSH.
        if ($_SERVER['PHP_SELF'] === '/gzip-close') {
            ob_end_clean();
        }
        echo '-last';
        break;
    case '/gzip-binary':
        ob_start('ob_gzhandler');
        $data = '';
        $random_state = 1;
        for ($i = 0; $i < 65536; ++$i) {
            $random_state = ($random_state * 1103515245 + 12345) & 0x7fffffff;
            $data .= chr(($random_state >> 16) & 255);
        }
        foreach ([0, 1, 7, 8, 31, 32, 255, 256, 16383, 65536] as $size) {
            echo substr($data, 0, $size);
            ob_flush();
            flush();
            flush();
        }
        break;
    case '/late-gzip':
        echo 'first';
        flush();
        ob_start('ob_gzhandler');
        echo '-last';
        break;
    case '/buffers':
        echo 'system';
        ob_start();
        echo '-user';
        flush();
        ob_start();
        echo '-nested';
        break;
    case '/forks':
        $a = fork(flush_from_fork('A'));
        $b = fork(flush_from_fork('B'));
        wait($a);
        wait($b);
        break;
    case '/large':
        for ($i = 0; $i < 16; ++$i) {
            echo str_repeat('x', 65536);
            flush();
        }
        break;
    case '/timeout':
        echo 'first';
        flush();
        sleep(5);
        echo '-last';
        break;
    case '/empty':
        flush();
        flush();
        break;
    default:
        echo 'ok';
}
