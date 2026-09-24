<?php

function flush_from_fork(string $tag): int
{
    $id = (int)get_running_fork_id();
    echo str_repeat($tag, 100000);
    flush();
    if ((int)get_running_fork_id() !== $id) {
        critical_error('flush changed the running fork ID');
    }
    return 1;
}

function checked_callback_flush(int $delay = 0): int
{
    $id = (int)get_running_fork_id();
    if ($delay > 0) {
        usleep($delay);
    }
    flush();
    if ((int)get_running_fork_id() !== $id) {
        critical_error('callback changed the running fork ID');
    }
    return 1;
}

/** @param future<int> $id */
function wait_callback_fork($id): int
{
    try {
        return (int)wait($id);
    } catch (Throwable $e) {
        critical_error('unexpected exception from callback fork: ' . $e->getMessage());
        return 0;
    }
}

function callback_child(int $depth): int
{
    if ($depth > 0) {
        $child = fork(callback_child($depth - 1));
        return wait_callback_fork($child);
    }
    echo 'child';
    return checked_callback_flush();
}

switch ($_SERVER['PHP_SELF']) {
    case '/callback-concurrent':
    case '/callback-preexisting':
        header_register_callback(function () {
            usleep(50000);
            header('X-Callback: ready');
            echo 'callback';
        });
        if ($_SERVER['PHP_SELF'] === '/callback-concurrent') {
            $first = fork(checked_callback_flush());
            $second = fork(checked_callback_flush());
            wait($first);
            wait($second);
        } else {
            $other = fork(checked_callback_flush(10000));
            checked_callback_flush();
            wait_callback_fork($other);
        }
        echo '-done';
        break;
    case '/callback-descendants-final':
    case '/callback-descendants':
        header_register_callback(function () {
            usleep(10000);
            header('X-Callback: ready');
            $child = fork(callback_child(2));
            wait_callback_fork($child);
            header('X-Too-Late: no');
            echo '-after';
        });
        if ($_SERVER['PHP_SELF'] === '/callback-descendants') {
            checked_callback_flush();
        }
        break;
    case '/callback-release-waiter':
        $other = fork(checked_callback_flush(10000));
        header_register_callback(function () use ($other) {
            usleep(50000); // The preexisting fork is now waiting inside flush().
            header('X-Callback: ready');
            echo 'first';
            checked_callback_flush(); // Commit headers and release the other fork.
            wait_callback_fork($other);
            echo '-after';
        });
        checked_callback_flush();
        break;
    case '/callback-finalize-running':
    case '/callback-finalize-committed':
        $commit = $_SERVER['PHP_SELF'] === '/callback-finalize-committed';
        header_register_callback(function () use ($commit) {
            header('X-Callback: ready');
            echo 'first';
            if ($commit) {
                checked_callback_flush();
            }
            usleep(50000);
            echo '-last';
        });
        fork(checked_callback_flush());
        // Finalization must await the callback, including output after a nested flush().
        break;
    case '/callback-exit-flush':
    case '/callback-exit-final':
        header_register_callback(function () {
            usleep(10000);
            header('X-Callback: ready');
            echo 'exit';
            exit(0);
        });
        if ($_SERVER['PHP_SELF'] === '/callback-exit-flush') {
            checked_callback_flush();
            echo 'unreachable';
        }
        break;
    case '/large-headers':
        for ($i = 0; $i < 24; ++$i) {
            header('X-Large-' . $i . ': ' . str_repeat('x', 49152));
        }
        echo 'large headers';
        flush();
        break;
    case '/content-length-flushed':
        $size = (int)$_GET['size'];
        header("Content-Length: $size");
        if ($size > 0) {
            echo 'first';
        }
        flush();
        usleep(100000);
        fwrite(fopen('php://stderr', 'w'), "content-length completed $size\n");
        break;
    case '/abort-flush':
    case '/ignore-abort-flush':
        $ignore = $_SERVER['PHP_SELF'] === '/ignore-abort-flush';
        ignore_user_abort($ignore);
        register_shutdown_function(function () {
            fwrite(fopen('php://stderr', 'w'), "abort flush shutdown completed\n");
        });
        echo 'first';
        flush();
        for ($i = 0; $i < 8; ++$i) {
            echo str_repeat('x', 4 * 1024 * 1024);
            flush();
        }
        usleep(100000);
        fwrite(fopen('php://stderr', 'w'), "ignore-abort flush completed\n");
        break;
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
