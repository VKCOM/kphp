<?php

if (isset($_SERVER["JOB_ID"])) {
  require_once "job_worker.php";
  do_job_worker();
} else {
  require_once "http_worker.php";
  do_http_worker();
}
