@ok
<?php
require_once 'kphp_tester_include.php';

interface Err {
  /**
   * @return string
   */
  public function getMessage() : string;
}

class GeneralErr implements Err {
  /** @var string */
  public $message;

  public function __construct(string $message) { $this->message = $message; }

  /**
   * @return string
   */
  public function getMessage() : string {
    return $this->message;
  }
}

class User {
  public $id = 0;
}

class UserLoadErr implements \Err {
	const ERR_CONNECT = -1;
	const ERR_INVALID_USER_ID = -2;

	private $user_id = 0;
	private $error_code = self::ERR_CONNECT;

	public function __construct(int $user_id, int $error_code) {
		$this->user_id = $user_id;
		$this->error_code = $error_code;
	}

/**
 * @return string
 */
	public function getMessage() : string {
		switch ($this->error_code) {
			case self::ERR_CONNECT: return 'Error connecting to engine';
			case self::ERR_INVALID_USER_ID: return "Invalid user_id: " . $this->user_id;
			default: return 'Unknown error';
		}
	}

/**
 * @return int
 */
	public function getUserId() {
		return $this->user_id;
	}
}

/** @return Err */
function e(string $message) {
  return new GeneralErr($message);
}

// -----------------

function logErr(Err $err): void {
  echo $err->getMessage(), "\n";
}

/** @return tuple(int, Err) */
function getCommonCount(int $with_user_id) {
	if ($with_user_id == 0)
		return tuple(0, e('Error connecting'));
	if ($with_user_id == 1)
		return tuple(0, e('Error calculating'));
  $count = $with_user_id;
	return tuple($count, null);
}

/**
 * @param int $with_user_id
 */
function demoCommonCount($with_user_id) {
  list($count, $err) = getCommonCount($with_user_id);
  if ($err) {
  	logErr($err);
  } else {
    print "count = $count\n";
  }
}

/** @return \tuple(User, UserLoadErr) */
function loadUser(int $user_id) {
	if ($user_id < 0)
		return tuple(null, new UserLoadErr($user_id, UserLoadErr::ERR_INVALID_USER_ID));
	if ($user_id == 0)
		return tuple(null, new UserLoadErr($user_id, UserLoadErr::ERR_CONNECT));
  $user = new User;
  $user->id = $user_id + 100;
  return tuple($user, null);
}

/**
 * @param int $user_id
 */
function demoLoadUser($user_id) {
  list($user, $err) = loadUser($user_id);
  if ($err) {
    $dummy1 = $user_id == $err->getUserId();
  	logErr($err);
    return;
  }
  echo $user->id, "\n";
}



demoCommonCount(-1);
demoCommonCount(0);
demoCommonCount(1);
demoCommonCount(2);

demoLoadUser(-1);
demoLoadUser(0);
demoLoadUser(1);
demoLoadUser(2);



