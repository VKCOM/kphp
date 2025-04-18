<?php

$RPC_CONFIG = [];

#ifndef K2
function rpc_send_requests(
    string $actor,
    array $arr,
    ?float $timeout,
    bool $ignore_answer,
    ?KphpRpcRequestsExtraInfo $requests_extra_info,
    bool $need_responses_extra_info
    ) : array {
      global $RPC_CONFIG;
      $port = $RPC_CONFIG[$actor];
      $conn = new_rpc_connection($actor, $port, 0, 5);
      $reqs_id = rpc_tl_query($conn, $arr, $timeout, $ignore_answer, $requests_extra_info, $need_responses_extra_info);
      return $reqs_id;
    }

function rpc_fetch_responses(array $query_ids): array {
  return rpc_tl_query_result($query_ids);
}

#endif


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
    public function work(string $output);
}

class ResumableWorker implements I {
    public function work(string $output) {
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
        fwrite(STDERR, $output);
    }
}

class RpcWorker implements I {
    protected int $port;

    public function __construct(int $port) {
        $this->port = $port;
    }

    public function work(string $output) {
        global $RPC_CONFIG;
        $RPC_CONFIG["localhost"] = $this->port;
        $reqs_id = rpc_send_requests("localhost", [["_" => "engine.sleep",
                                                    "time_ms" => 120]], null, false, null, false);
        $resp = rpc_fetch_responses($reqs_id)[0];
        assert($resp['result']);
        fwrite(STDERR, $output);
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
     case "flush_and_header_register_callback_flush_inside_callback":
        echo "Zero ";
        header_register_callback(function () {
            echo "Two ";
            flush();
            sleep(2);
            echo "Three ";
        });
        echo "One ";
        return;
     case 'flush_and_header_register_callback_invoked_after_flush':
        header_register_callback(function () {
            echo "One ";
        });
        echo "Zero ";
        flush();
        sleep(2);
        echo "Two ";
        return;
     case 'flush_and_header_register_callback_no_double_invoked_after_flush':
        header_register_callback(function () {
            echo "One ";
        });
        echo "Zero ";
        flush();
        sleep(2);
        echo "Two ";
        flush();
        sleep(2);
        echo "Three ";
        return;
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
} else if ($_SERVER["PHP_SELF"] === "/test_headers_sent") {
    switch($_GET["type"]) {
        case "flush":
            echo (int)headers_sent();
            flush();
            echo (int)headers_sent();
            break;
        case "shutdown":
            if ((int)$_GET['flush']) {
                flush();
            }
            register_shutdown_function(function() {
              fwrite(STDERR, "headers_sent() after shutdown callback returns " . var_export(headers_sent(), true) . "\n");
            });
            break;
    }
} else if ($_SERVER["PHP_SELF"] === "/test_ignore_user_abort") {
    register_shutdown_function('shutdown_function');
    /** @var I */
    $worker = null;
    $msg = "";
    switch($_GET["type"]) {
     case "rpc":
        $worker = new RpcWorker(intval($_GET["port"]));
        $msg = "test_ignore_user_abort/finish_rpc_work_" . $_GET["level"] . "\n";
        break;
     case "resumable":
        $worker = new ResumableWorker;
        $msg = "test_ignore_user_abort/finish_resumable_work_" . $_GET["level"] . "\n";
        break;
     default:
        echo "ERROR"; return;
    }
    switch($_GET["level"]) {
     case "no_ignore":
        $worker->work($msg);
        break;
     case "ignore":
        ignore_user_abort(true);
        $worker->work($msg);
        fwrite(STDERR, "test_ignore_user_abort/finish_ignore_" . $_GET["type"] . "\n");
        ignore_user_abort(false);
        break;
     case "multi_ignore":
        ignore_user_abort(true);
        $worker->work($msg);
        $worker->work($msg);
        fwrite(STDERR, "test_ignore_user_abort/finish_multi_ignore_" . $_GET["type"] . "\n");
        ignore_user_abort(false);
        break;
     case "nested_ignore":
        ignore_user_abort(true);
        ignore_user_abort(true);
        $worker->work($msg);
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
#ifndef K2
    require_once "test_oom_handler.php";
#endif
} else if ($_SERVER["PHP_SELF"] === "/test_header_register_callback") {
    header_register_callback(function () {
        global $_GET;
        switch($_GET["act_in_callback"]) {
         case "rpc":
            $msg = "test_header_register_callback/rpc_in_callback\n";
            (new RpcWorker(intval($_GET["port"])))->work($msg);
            break;
         case "exit":
            $msg = "test_header_register_callback/exit_in_callback";
            exit($msg);
         default:
            echo "ERROR";
            return;
        }
    });
} else {
    if ($_GET["hints"] === "yes") {
        send_http_103_early_hints(["Content-Type: text/plain or application/json", "Link: </script.js>; rel=preload; as=script"]);
        sleep(2);
    }
    echo "Hello world!";
}
