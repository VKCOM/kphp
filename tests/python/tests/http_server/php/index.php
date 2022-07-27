<?php

/** @kphp-immutable-class */
class A {
  /** @var int */
  public $a = 42;
  /** @var string */
  public $b = "hello";
}

/**
 * @kphp-required
 */
function shutdown_function() {
    fwrite(STDERR, "shutdown_function was called\n");
}

interface I {
    public function work();
}

class ResumableWorker implements I {
    public function work() {
        $job = function($x) {
                sched_yield_sleep(2);
                return $x;
        };
        $futures = [];
        for ($i = 0; $i < 10; ++$i) {
            $futures[] = fork($job($i));
        }
        wait_multi($futures);
        fwrite(STDERR, "test_ignore_user_abort/finish_resumable_work\n");
    }
}

class RpcWorker implements I {
    protected int $port;

    public function __construct(int $port) {
        $this->port = $port;
    }

    public function work() {
        fwrite(STDERR, $this->port . "\n");
        $job = function() {
            $conn = new_rpc_connection('localhost', $this->port, 0, 3);
            $req_id = rpc_tl_query_one($conn, ["_" => "engine.sleep",
                                                "time_ms" => 60]);
            $resp = rpc_tl_query_result_one($req_id);
        };
        for ($i = 0; $i < 3; ++$i) {
            $job();
        }
        fwrite(STDERR, "test_ignore_user_abort/finish_rpc_work\n");
   }
}


if ($_SERVER["PHP_SELF"] === "/ini_get") {
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
        fwrite(STDERR, "test_ignore_user_abort/finish_ignore\n");
        ignore_user_abort(false);
        break;
     case "multi_ignore":
        ignore_user_abort(true);
        ignore_user_abort(true);
        $worker->work();
        ignore_user_abort(false);
        fwrite(STDERR, "test_ignore_user_abort/finish_multi_ignore\n");
        ignore_user_abort(false);
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
} else {
  echo "Hello world!";
}
