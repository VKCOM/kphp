@ok
<?php

require_once 'kphp_tester_include.php';

// listen different ports for php and kphp 
$port = 8080;
if (kphp)
  $port = 8081;

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

function test_file_option() {
  global $port;
  $filename_in = "input_test_file.txt";
  $filename_out = "output_test_file.txt";
  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");
  var_dump(fwrite($fh_in, "hello world!"));

  if (kphp)
    // got a different behavior for fopen() in "w+" mode
    // without rewind() in kphp the output file will be empty
    rewind($fh_in);

  $c = curl_init("http://localhost:$port");
  $cmd = "php -S localhost:$port ./$filename_in";

  var_dump(curl_setopt($c, CURLOPT_FILE, $fh_out));
  var_dump(curl_setopt($c, CURLOPT_VERBOSE, 1)); // get all information about connections
  $server = new Process($cmd, $port);
  usleep(1000 * 200);
  var_dump(curl_exec($c)); // true, if the connection to the localhost is successfull

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

function test_infile_option() {
  global $port;
  $filename_in = "test_file.txt";
  $filename_out = "router.php";

  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");
  var_dump(fwrite($fh_in, "hello world from input file!"));
  var_dump(fwrite($fh_out, "<?php\n\$putdata = file_get_contents('php://input');\n"));
  var_dump(fwrite($fh_out, "var_dump(\$putdata);"));
  rewind($fh_in);

  if (kphp)
    rewind($fh_out);

  $c = curl_init("http://localhost:$port");
  $cmd = "php -S localhost:$port ./$filename_out";

  var_dump(curl_setopt($c, CURLOPT_POST, true));
  var_dump(curl_setopt($c, CURLOPT_INFILESIZE, filesize("./$filename_in")));
  var_dump(curl_setopt($c, CURLOPT_INFILE, $fh_in));
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

function test_writeheader_option() {
  global $port, $cmd;
  $filename_in = "router.php";
  $filename_out = "test_file.txt";

  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");
  var_dump(fwrite($fh_in, "<?php\necho 'hello world\n';"));

  if (kphp)
    rewind($fh_in);

  $c = curl_init("http://localhost:$port");
  $cmd = "php -S localhost:$port ./$filename_in";
  var_dump(curl_setopt($c, CURLOPT_WRITEHEADER, $fh_out));
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

test_file_option();
test_infile_option();
test_writeheader_option();