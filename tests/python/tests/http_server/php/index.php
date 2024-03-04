<?php

/**
 * @kphp-required
 */
function shutdown_function() {
    fwrite(STDERR, "shutdown_function was called\n");
}

function assert($flag) {
    if ($flag !== true) {
        critical_error("failed assert");
    }
}

interface I {
    public function work();
}

class ResumableWorker implements I {
    public function work() {
        $job = function() {
                sched_yield_sleep(2);
                return true;
        };
        $futures = [];
        for ($i = 0; $i < 3; ++$i) {
            $futures[] = fork($job());
        }
        $responses = wait_multi($futures);
        foreach ($responses as $resp) {
            assert($resp);
        }
        fwrite(STDERR, "test_ignore_user_abort/finish_resumable_work_" . $_GET["level"] . "\n");
    }
}

class RpcWorker implements I {
    protected int $port;

    public function __construct(int $port) {
        $this->port = $port;
    }

    public function work() {
        $conn = new_rpc_connection('localhost', $this->port, 0, 5);
        $req_id = rpc_tl_query_one($conn, ["_" => "engine.sleep",
                                                    "time_ms" => 120]);
        $resp = rpc_tl_query_result_one($req_id);
        assert($resp['result']);
        fwrite(STDERR, "test_ignore_user_abort/finish_rpc_work_" . $_GET["level"] . "\n");
   }
}

if (isset($_SERVER["JOB_ID"])) {
  require_once "job_worker.php";
} else if ($_SERVER["PHP_SELF"] === "/ini_get") {
  echo ini_get($_SERVER["QUERY_STRING"]);
} else if (substr($_SERVER["PHP_SELF"], 0, 12) === "/test_limits") {
  echo json_encode($_SERVER);
} else if ($_SERVER["PHP_SELF"] === "/sleep") {
  $sleep_time = (int)$_GET["time"];
  echo "before sleep $sleep_time\n";
  fwrite(STDERR, "sleeping for $sleep_time sec ...\n");
  sleep($sleep_time);
  fwrite(STDERR, "wake up!");
  echo "after sleep";
} else if ($_SERVER["PHP_SELF"] === "/store-in-instance-cache") {
  echo instance_cache_store("test_key" . rand(), new A);
} else if ($_SERVER["PHP_SELF"] === "/test_zstd") {
  $res = "";
  switch($_GET["type"]) {
    case "uncompress":
      $res = zstd_uncompress((string)file_get_contents("in.dat")); break;
    case "uncompress_dict":
      $res = zstd_uncompress_dict((string)file_get_contents("in.dat"), (string)file_get_contents($_GET["dict"])); break;
    case "compress":
      $res = zstd_compress((string)file_get_contents("in.dat"), (int)$_GET["level"]); break;
    case "compress_dict":
      $res = zstd_compress_dict((string)file_get_contents("in.dat"), (string)file_get_contents($_GET["dict"])); break;
    default:
      echo "ERROR"; return;
  }
  file_put_contents("out.dat", $res === false ? "false" : $res);
  echo "OK";
} else if ($_SERVER["PHP_SELF"] === "/test_script_flush") {
    switch($_GET["type"]) {
     case "one_flush":
        echo "Hello ";
        flush();
        sleep(2);
        echo "world";
        return;

     case "few_flush":
        echo "This ";
        flush();
        sleep(2);
        echo "is big ";
        flush();
        sleep(2);
        echo "message";
        return;

     case "transfer_encoding_chunked":
        header("Transfer-Encoding: chunked");

        $chunk = "Hello world";
        echo sprintf("%x\r\n", strlen($chunk)) . $chunk . "\r\n";
        flush();
        sleep(2);

        $chunk = "Bye world";
        echo sprintf("%x\r\n", strlen($chunk)) . $chunk . "\r\n";
        return;

     case "error_on_flush":
        echo "Start work";
        flush();
        sleep(2);
        throw new Exception('Exception');
    }

    echo "OK";
} else if ($_SERVER["PHP_SELF"] === "/test_script_gzip_header") {
    switch($_GET["type"]) {
      case "gzip":
        ob_start("ob_gzhandler");
        echo 'OK';
        break;
      case "reset":
        ob_start("ob_gzhandler");
        echo 'OK';
        ob_end_flush();
        break;
      case "ignore-second-handler":
        header('Transfer-Encoding: chunked');
        $chunk = "OK";
        printf("%X\r\n", strlen($chunk));
        echo $chunk . "\r\n";
        flush(); // flush headers so the handler below should not affect them anymore
        ob_start("ob_gzhandler");
        $chunk = "OK";
        printf("%X\r\n", strlen($chunk));
        echo $chunk . "\r\n";
        printf("0\r\n\r\n");

        break;
    }
} else if ($_SERVER["PHP_SELF"] === "/test_ignore_user_abort") {
    register_shutdown_function('shutdown_function');
    /** @var I */
    $worker = null;
    switch($_GET["type"]) {
     case "rpc":
        $worker = new RpcWorker(intval($_GET["port"]));
        break;
     case "resumable":
        $worker = new ResumableWorker;
        break;
     default:
        echo "ERROR"; return;
    }
    switch($_GET["level"]) {
     case "no_ignore":
        $worker->work();
        break;
     case "ignore":
        ignore_user_abort(true);
        $worker->work();
        fwrite(STDERR, "test_ignore_user_abort/finish_ignore_" . $_GET["type"] . "\n");
        ignore_user_abort(false);
        break;
     case "multi_ignore":
        ignore_user_abort(true);
        $worker->work();
        $worker->work();
        fwrite(STDERR, "test_ignore_user_abort/finish_multi_ignore_" . $_GET["type"] . "\n");
        ignore_user_abort(false);
        break;
     case "nested_ignore":
        ignore_user_abort(true);
        ignore_user_abort(true);
        $worker->work();
        ignore_user_abort(false);
        fwrite(STDERR, "test_ignore_user_abort/finish_nested_ignore_" . $_GET["type"] . "\n");
        ignore_user_abort(false);
     default:
        echo "ERROR"; return;
    }
    echo "OK";
} else if ($_SERVER["PHP_SELF"] === "/test_runtime_config") {
    $config = kphp_get_runtime_config();
    switch ($_GET["mode"]) {
     case "read_only":
         echo "name : " . $config["name"] . " ";
         echo "version : " . $config["version"] . " ";
         break;
     case "modify":
         $config["version"] = "1.0.1";
         echo "modified version : " . $config["version"] . " ";
         echo "old version : " . kphp_get_runtime_config()["version"] . " ";
         break;
     default:
     echo "ERROR"; return;
    }
    echo "OK";
} else if ($_SERVER["PHP_SELF"] === "/test_big_post_data") {
    $keys = array_keys($_POST);
    if ($keys) {
        $value = array_shift($keys);
        $res = strlen($value);
    } else {
        $res = 0;
    }
    echo json_encode(['len' => $res]);
} else if ($_SERVER["PHP_SELF"] === "/pid") {
    echo "pid=" . posix_getpid();
} else if ($_SERVER["PHP_SELF"] === "/test_script_errors") {
  critical_error("Test error");
} else if ($_SERVER["PHP_SELF"] === "/test_oom_handler") {
  require_once "test_oom_handler.php";
} else {
    if ($_GET["hints"] === "yes") {
        send_http_103_early_hints(["Content-Type: text/plain or application/json", "Link: </script.js>; rel=preload; as=script"]);
        sleep(2);
    }
    echo "Hello world!";
}
