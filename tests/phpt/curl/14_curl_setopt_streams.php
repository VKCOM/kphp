@ok
<?php

require_once 'kphp_tester_include.php';

$is_kphp = true;
#ifndef KPHP
$is_kphp = false;
#endif

class Process {
  private $pid;
  private $command;
  private $outputs;

  public function __construct($cl=false) {
    if ($cl != false) {
      $this->command = $cl;
      $this->runcmd();
    }
    $this->outputs = array();
  }

  private function runcmd() {
    $command = 'nohup ' . $this->command .' > /dev/null 2>&1 & echo $!';
    exec($command, $this->outputs);
    $this->pid = (int)$this->outputs[0];
  }

  public function getpid() {
    exec("(ps aux | grep '[p]hp -S localhost' | awk '{print $2}')", $op);
    $this->pid = $op[0];
    return $this->pid;
  }

  public function status() {
    $command = 'ps -p ' . $this->pid;
    exec($command, $op);
    if (!isset($op[1])) 
      return false;
    return true;
  }

  public function start() {
    if ($this->command != '') 
      $this->runcmd();
    else 
      return true;
  }

  public function stop() {
    $this->getpid();
    $command = 'kill '. $this->pid;
    exec($command);
    if ($this->status() == false) 
      return true;
    return false;
  }
}

function test_file_option() {
  global $is_kphp;
  $filename_in = "input_test_file.txt";
  $filename_out = "output_test_file.txt";
  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");
  var_dump(fwrite($fh_in, "hello world!"));

  // listen different ports for php and kphp 
  $port = 8080;
  if ($is_kphp) {
    $port = 8081;
    // got a different behavior for fopen() in "w+" mode
    // without rewind() in kphp the output file will be empty
    rewind($fh_in);
  }

  $c = curl_init("http://localhost:$port");
  $cmd = "php -S localhost:$port ./$filename_in";

  var_dump(curl_setopt($c, CURLOPT_FILE, $fh_out));
  var_dump(curl_setopt($c, CURLOPT_VERBOSE, 1)); // get all information about connections
  $server = new Process($cmd);
  usleep(1000 * 30); // sleep for 30 ms
  var_dump(curl_exec($c)); // true, if the connection to the localhost is successfull

  var_dump(rewind($fh_out));
  var_dump(filesize("./$filename_out"));
  var_dump(fread($fh_out, filesize("./$filename_out")));
  var_dump(fclose($fh_in));
  var_dump(fclose($fh_out));
  curl_close($c);
  exec("rm ./$filename_in ./$filename_out");
  var_dump($server->stop());
}

function test_infile_option() {
  global $is_kphp;
  $filename_in = "test_file.txt";
  $filename_out = "server_file.php";

  $fh_in = fopen("$filename_in", "w+");
  $fh_out = fopen("$filename_out", "w+");
  var_dump(fwrite($fh_in, "hello world from input file!"));
  var_dump(fwrite($fh_out, "<?php\n\$putdata = file_get_contents('php://input');\n"));
  var_dump(fwrite($fh_out, "var_dump(\$putdata);"));
  rewind($fh_in);

  // listen different ports for php and kphp 
  $port = 8080;
  if ($is_kphp) {
    $port = 8081;
    rewind($fh_out);
  }

  $c = curl_init("http://localhost:$port");
  $cmd = "php -S localhost:$port ./$filename_out";

  var_dump(curl_setopt($c, CURLOPT_POST, true));
  var_dump(curl_setopt($c, CURLOPT_INFILESIZE, filesize("./$filename_in")));
  var_dump(curl_setopt($c, CURLOPT_INFILE, $fh_in));
  var_dump(curl_setopt($c, CURLOPT_VERBOSE, 1));

  $server = new Process($cmd);
  usleep(1000 * 30);
  var_dump(curl_exec($c));

  var_dump(fclose($fh_in));
  var_dump(fclose($fh_out));
  curl_close($c);
  exec("rm ./$filename_in ./$filename_out");
  var_dump($server->stop());
}

test_file_option();
test_infile_option();