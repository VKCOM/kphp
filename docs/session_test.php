<?php

#ifndef KPHP
spl_autoload_register(function($class) {
  $rel_filename = trim(str_replace('\\', '/', $class), '/') . '.php';
  $filename = __DIR__ . '/..' . '/' . $rel_filename;
  if (file_exists($filename)) {
    require_once $filename;
  }
}, true, true);

// todo require kphp_polyfills or install them using Composer
// see https://github.com/VKCOM/kphp-polyfills
// (or don't do this, since job workers just don't work in plain PHP,
//  and in practice, you should provide local fallback; this demo is focused to be KPHP-only :)
require_once '/some/where/kphp-polyfills/kphp_polyfills.php';
#endif


class MyRequest extends \JobWorkers\JobWorkerSimple {
  /** @var false|string */
  public $session_id;

  /** @var array<mixed> */
  public $predefined_consts;

  /** @var mixed */
  public $session_array;

  /** @var bool */
  public $must_sleep;

  /**
   * @param array<mixed> $predefined_consts
   * @param mixed $session_array
   * @param false|string $id
   * @param bool $must_sleep
  */
  public function __construct($predefined_consts, $session_array, $id, $must_sleep) {
    $this->session_id = $id;
    $this->predefined_consts = $predefined_consts;
    $this->session_array = $session_array;
    $this->must_sleep = $must_sleep;
  }

  function handleRequest(): ?\KphpJobWorkerResponse {
    $response = new MyResponse();
    session_id($this->session_id);
    
    $response->start_time = microtime(true);
    session_start($this->predefined_consts);
    $response->session_status = (session_status() == 2);
    $response->end_time = microtime(true);
    
    $response->session_id = session_id();
    if (!$response->session_status) {
      return $response;
    }
    
    $session_array_before = unserialize(session_encode());
    session_decode(serialize(array_merge($session_array_before, $this->session_array)));
    // or: $_SESSION = array_merge($this->session_array, $_SESSION);
    $response->session_array = unserialize(session_encode());
    // or: $response->session_array = $_SESSION;

    if ($this->must_sleep) {
      usleep(2 * 100000);
    }
    session_commit();
    
    return $response;
  }
}

class MyResponse implements \KphpJobWorkerResponse {
  /** @var float */
  public $start_time;

  /** @var float */
  public $end_time;

  /** @var bool */
  public $session_status;

  /** @var string|false */
  public $session_id;

  /** @var mixed */
  public $session_array;
}


if (PHP_SAPI !== 'cli' && isset($_SERVER["JOB_ID"])) {
  handleKphpJobWorkerRequest();
} else {
  handleHttpRequest();
}

function handleHttpRequest() {
  if (!\JobWorkers\JobLauncher::isEnabled()) {
    echo "JOB WORKERS DISABLED at server start, use -f 2 --job-workers-ratio 0.5", "\n";
    return;
  }

  $timeout = 0.5;
  $to_write = ["first_message" => "hello"];

  // ["save_path" => "/Users/marat/Desktop/sessions/"]
  $main_request = new MyRequest([], $to_write, false, false);
  $main_job_id = \JobWorkers\JobLauncher::start($main_request, $timeout);

  $main_response = wait($main_job_id);
  $session_id = false;

  if ($main_response instanceof MyResponse) {
    echo "Created main session:<br>";
    var_dump($main_response->session_status);
    var_dump($main_response->session_id);
    var_dump($main_response->session_array);
    $session_id = $main_response->session_id;
    echo "<br><br>";
  } else {
    return;
  }
  
  $main_request = new MyRequest([], ["second_message" => "world"], $session_id, true);
  $main_job_id = \JobWorkers\JobLauncher::start($main_request, $timeout);

  $new_request = new MyRequest([], ["third_message" => "buy"], $session_id, false);
  $new_job_id = \JobWorkers\JobLauncher::start($new_request, $timeout);

  $new_response = wait($new_job_id);
  $main_response = wait($main_job_id);

  if ($main_response instanceof MyResponse) {
    echo "<br>Opened session:<br>";
    var_dump($main_response->session_status);
    var_dump($main_response->session_id);
    var_dump($main_response->session_array);
    echo "<br><br>";
  }

  if ($new_response instanceof MyResponse) {
    echo "<br>Opened session:<br>";
    var_dump($new_response->session_status);
    var_dump($new_response->session_id);
    var_dump($new_response->session_array);
    echo "<br><br>";
  }
}

function handleKphpJobWorkerRequest() {
  $kphp_job_request = kphp_job_worker_fetch_request();
  if (!$kphp_job_request) {
    warning("Couldn't fetch a job worker request");
    return;
  }

  if ($kphp_job_request instanceof \JobWorkers\JobWorkerSimple) {
    // simple jobs: they start, finish, and return the result
    $kphp_job_request->beforeHandle();
    $response = $kphp_job_request->handleRequest();
    if ($response === null) {
      warning("Job request handler returned null for " . get_class($kphp_job_request));
      return;
    }
    kphp_job_worker_store_response($response);

  } else if ($kphp_job_request instanceof \JobWorkers\JobWorkerManualRespond) {
    // more complicated jobs: they start, send a result in the middle (here get get it) — and continue working
    $kphp_job_request->beforeHandle();
    $kphp_job_request->handleRequest();
    if (!$kphp_job_request->wasResponded()) {
      warning("Job request handler didn't call respondAndContinueExecution() manually " . get_class($kphp_job_request));
    }

  } else if ($kphp_job_request instanceof \JobWorkers\JobWorkerNoReply) {
    // background jobs: they start and never send any result, just continue in the background and finish somewhen
    $kphp_job_request->beforeHandle();
    $kphp_job_request->handleRequest();

  } else {
    warning("Got unexpected job request class: " . get_class($kphp_job_request));
  }
}
