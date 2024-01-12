@ok
<?php

require_once 'kphp_tester_include.php';

// this test is not ready yet
// only for testing kphp with new order of stages in compiler.cpp

$filename_in = "input_test_file.txt";
$filename_out = "output_test_file.txt";
$port = 8082;
if (kphp)
  $port = 8083;
$cmd = "php -S localhost:$port ./$filename_in";
$text = <<<EOD
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. 
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. 
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. 
Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.
EOD;

class Process {
  private $pid;
  private $cmd;
  private $port;

  public function __construct($cmd, $port) {
    if ($cmd) {
      $this->cmd = 'nohup ' . $cmd . ' > /dev/null 2>&1 & echo $!';
      $this->port = $port;
      exec($this->cmd);
    } else {
        throw new Exception("The command for running a server on the port $port is empty!\n");
    }
  }

  public function update_pid() {
    exec("lsof -i:$this->port | awk 'NR==2' | awk '{print $2}'", $op);
    if (!isset($op[0])) {
        throw new Exception("Couldn't define the pid of running server on the port $this->port");
    }
    return $this->pid = $op[0];
  }

  public function status() {
    $cmd = 'ps -p ' . $this->pid;
    exec($cmd, $op);
    return (isset($op[1]));
  }

  public function start() {
    $code = false;
    if ($this->cmd)
      exec($this->cmd, $op, $code);
    return $code;
  }

  public function stop() {
    $this->update_pid();
    $cmd = 'kill '. $this->pid;
    exec($cmd);
    return $this->status() == false;
  }
}

function test_writefunction_option() {
  global $filename_in, $filename_out, $port, $cmd, $text;
  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");

  var_dump(fwrite($fh_in, "$text"));
  if (kphp)
    rewind($fh_in);

  $callback = function ($c, $str) use ($fh_out) {
    fwrite($fh_out, $str);
    return strlen($str);
  };

  $c = curl_init("http://localhost:$port");
  var_dump(curl_setopt($c, CURLOPT_WRITEFUNCTION, $callback));
  var_dump(curl_setopt($c, CURLOPT_VERBOSE, 1));
  $server = new Process($cmd, $port);
  usleep(1000 * 200);
  var_dump(curl_exec($c));

  var_dump(rewind($fh_out));
  var_dump(filesize("./$filename_out"));
  var_dump(fread($fh_out, filesize("./$filename_out")));
  var_dump(fclose($fh_in));
  var_dump(fclose($fh_out));
  curl_close($c);
  exec("rm ./$filename_in ./$filename_out");
  var_dump($server->stop());
  usleep(1000 * 200);
}

function test_headerfunction_option() {
  global $filename_in, $filename_out, $port, $cmd;
  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");

  var_dump(fwrite($fh_in, "hello world\n"));
  if (kphp)
    rewind($fh_in);

  $callback = function ($c, $str) use ($fh_out) {
    fwrite($fh_out, $str);
    return strlen($str);
  };

  $c = curl_init("http://localhost:$port");
  var_dump(curl_setopt($c, CURLOPT_HEADERFUNCTION, $callback));
  var_dump(curl_setopt($c, CURLOPT_VERBOSE, 1));
  $server = new Process($cmd, $port);
  usleep(1000 * 200);
  var_dump(curl_exec($c));

  var_dump(rewind($fh_out));
  var_dump(fread($fh_out, 17));
  var_dump(fclose($fh_in));
  var_dump(fclose($fh_out));
  curl_close($c);
  exec("rm ./$filename_in ./$filename_out");
  var_dump($server->stop());
  usleep(1000 * 200);
}

function test_progressfunction_option() {
  global $filename_in, $filename_out, $port, $text;
  $filename_router = "router.php";
  $fh_router = fopen($filename_router, "w+");
  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");

  var_dump(fwrite($fh_in, "$text"));
  // Write a simple script that tells the client to download the contents of the input_test_file.txt 
  // when requesting the server
  var_dump(fwrite(
    $fh_router, 
    "<?php\n
    \$file='$filename_in';\n
    header('Content-Type:text/plain');\n
    header('Content-Disposition:attachment;filename=\$file');\n
    header('Content-Length: ' . filesize(\$file));\n
    echo file_get_contents(\$file);\n
    exit;\n
    ?>"
  ));

  if (kphp) {
    rewind($fh_in);
    rewind($fh_router);
  }

  $progress_callback = function($resource, $download_size, $downloaded, $upload_size, $uploaded) {
    if($download_size > 0) {
      echo round(($downloaded * 100) / $download_size);
      echo "\n";
    }
    usleep(1000*5);
    return 0;
  };

  $c = curl_init("http://localhost:$port");
  $cmd = "php -S localhost:$port ./$filename_router";
  var_dump(curl_setopt($c, CURLOPT_FILE, $fh_out));
  var_dump(curl_setopt($c, CURLOPT_PROGRESSFUNCTION, $progress_callback));
  var_dump(curl_setopt($c, CURLOPT_NOPROGRESS, false));
  var_dump(curl_setopt($c, CURLOPT_VERBOSE, 1));
  $server = new Process($cmd, $port);
  usleep(1000 * 200);
  var_dump(curl_exec($c));

  var_dump(rewind($fh_out));
  var_dump(filesize("./$filename_out"));
  var_dump(fread($fh_out, filesize("./$filename_out")));
  var_dump(fclose($fh_in));
  var_dump(fclose($fh_out));
  var_dump(fclose($fh_router));
  curl_close($c);
  exec("rm ./$filename_in ./$filename_out ./$filename_router");
  var_dump($server->stop());
  usleep(1000 * 200);
}

function test_readfunction_option() {
  global $port, $text;
  $filename_in = "test_file.txt";
  $filename_out = "router.php";

  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");
  var_dump(fwrite($fh_in, "$text"));
  var_dump(fwrite($fh_out, "<?php\n\$putdata = file_get_contents('php://input');\n"));
  var_dump(fwrite($fh_out, "var_dump(\$putdata);"));
  rewind($fh_in);

  if (kphp)
    rewind($fh_out);

  $callback = function($c, $fh, $length) {
    $result = fread($fh, $length);
    return $result ?: "";
  };

  $c = curl_init("http://localhost:$port");
  $cmd = "php -S localhost:$port ./$filename_out";

  var_dump(curl_setopt($c, CURLOPT_PUT, true));
  var_dump(curl_setopt($c, CURLOPT_INFILESIZE, filesize("./$filename_in")));
  var_dump(curl_setopt($c, CURLOPT_INFILE, $fh_in));
  var_dump(curl_setopt($c, CURLOPT_READFUNCTION, $callback));
  var_dump(curl_setopt($c, CURLOPT_VERBOSE, 1));

  $server = new Process($cmd, $port);
  usleep(1000 * 200);
  var_dump(curl_exec($c));

  var_dump(fclose($fh_in));
  var_dump(fclose($fh_out));
  curl_close($c);
  exec("rm ./$filename_in ./$filename_out");
  var_dump($server->stop());
  usleep(1000 * 200);
}

test_writefunction_option();
test_headerfunction_option();
test_progressfunction_option();
test_readfunction_option();