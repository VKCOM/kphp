@ok
<?php

interface AI {}

class A1 implements AI {
	public $a1 = "a1";
}

class A2 implements AI {
	public $a2 = "a2";
}

class Request implements KphpJobWorkerRequest {
	/** @var AI */
	public $ai = null;

	/** @var string|false */
	public $session_id;

	function __construct() {
		$this->session_id = false;
	}

	function handleRequest(): ?KphpJobWorkerResponse {
		$response = new Response();
		$status = session_start();
		if ($status) {

		}
		$response->session_id = 
	}
}

class Response implements KphpJobWorkerResponse {
	/** @var AI */
	public $ai = null;

	/** @var string|false */
	public $session_id = false;
}


$request1 = new Request();
$request1->ai = new A1();
$request2 = Request();
$request2->ai = new A2();
$ids = array();
kphp_job_worker_start($request, -1);


