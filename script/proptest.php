<?php
define('MARKER', "\0");

class StreamHolder {
    /** @var ComponentStream $stream*/
    public $stream;

    /** @param ComponentStream $s*/
    public function __construct($s) {
        $this->stream = $s;
    }

    public function close_stream() {
        component_close_stream($this->stream);
    }

    /** @param ComponentStream $stream*/
    public function reset($stream) {
        $this->stream = $stream;
    }
}

class Reader extends StreamHolder {
    public $count = 0;

    /** @param ComponentStream $s*/
    public function __construct($s) {
        parent::__construct($s);
        $this->count = 0;
    }

    public function is_read_closed() {
        return $this->stream->is_read_closed();
    }

    public function read_byte() {
        $str = component_stream_read_exact($this->stream, 1);
        $this->count++;
        return $str;
    }
}

class Writer extends StreamHolder {
    public $count = 0;

    /** @param ComponentStream $s*/
    public function __construct($s) {
        parent::__construct($s);
        $this->count = 0;
    }

    public function is_write_closed() {
        return $this->stream->is_write_closed();
    }

    public function write_exact($message) {
        $this->count += component_stream_write_exact($this->stream, $message);
        return;
    }
}

class BiDirectStream {
    /** @var Reader $reader*/
    public $reader;
    /** @var Writer $writer*/
    public $writer;

    public $name;

    public $last_saved_header;


    /** @param ComponentStream $stream*/
    public function __construct($stream, $name) {
        $this->reader = new Reader($stream);
        $this->writer = new Writer($stream);
        $this->name = $name;
    }

    public function read_full_header() {
        $m = $this->reader->read_byte();
        if ($m !== MARKER && !$this->reader->is_read_closed()) {
            critical_error("here can be only marker, but got " . $m);
        }
        return $this->read_header();
    }

    public function read_header() {
        warning("read header call");

        $forward_id = byte_to_int($this->reader->read_byte());
        $seed = byte_to_int($this->reader->read_byte());
        $header = ["forward_id" => $forward_id, "seed" => $seed];
        return $header;
    }

    public function get_last_header() {
        return $this->last_saved_header;
    }

    public function save_header($header) {
        $this->last_saved_header = $header;
    }

    public function write_full_header($header) {
        $str = "";
        $str .= MARKER;
        $str .= int_to_byte($header["forward_id"] + 1);
        $str .= int_to_byte($header["seed"]);
        debug_print_string($str);
        $this->writer->write_exact($str);

//         if ($this->writer->write_exact($str) != 3) {
//             critical_error("cannot write header");
//         }
    }

    public function reopen() {
        $this->close_stream();
        $stream = component_open_stream($this->name);
        if (is_null($stream)) {
            warning("cannot open stream to " . $this->$name);
            return false;
        }
        $this->reader->reset($stream);
        $this->writer->reset($stream);
        $this->reader->count = 0;
        $this->writer->count = 0;
        return true;
    }

    public function close_stream() {
        $this->reader->close_stream();
    }

    public function is_open() {
        return !$this->reader->is_read_closed() && !$this->writer->is_write_closed();
    }

    public function get_stat() {
        return [$this->reader->count, $this->writer->count];
    }
}

/**
 * @param BiDirectStream $prev
 * @param BiDirectStream $next
 */
function print_stat($prev, $next) {
    $a = $prev->get_stat();
    $b = $next->get_stat();
    warning("read from prev " . $a[0] .". write to prev ". $a[1]);
    warning("write to next " . $b[1] .". read from next ". $b[0]);

}

function setup_die_timer() {
    global $scenario;
    $type = $scenario["die"]["type"];
    if ($type === "NoDie") {
        return;
    }
    $timeout = intval($scenario["die"]["time_ms"]);
    set_timer($timeout, function() {die();});
}


function get_destination($forward_id)
{
    global $scenario;
    $len = count($scenario["forward"]["num"]);
    return $scenario["forward"]["names"][0];
}

/**
 * @param BiDirectStream $stream
  */
function echo_bytes($stream) {
    while(!$stream->reader->is_read_closed() && !$stream->writer->is_write_closed()) {
        backward_byte($stream, $stream);
    }
}

/**
 * @param BiDirectStream $src
 * @param BiDirectStream $dst
 */
function stream_header($src, $dst) {
    $forward_header = $src->read_header();
    $dst->write_full_header($forward_header);
    $dst->save_header($forward_header);

    $backward_header = $dst->read_full_header();
    $src->write_full_header($backward_header);
    $src->save_header($backward_header);
}

/**
 * @param BiDirectStream $src
 * @param BiDirectStream $dst
 */
function forward_byte($src, $dst) {
    $str = $src->reader->read_byte();
    // cannot open stream to dst. Try reopen once
    if ($dst->writer->is_write_closed()) {
        $successful = $dst->reopen();
        if (!$successful) {
            return false;
        }
        warning("successful reopen to forward");
        if ($str !== MARKER) {
            warning("send last header");
            // If there is no new message still need send last header
            $last_header = $dst->get_last_header();
            $dst->write_full_header($last_header);
            $dst->read_full_header();
        }
    }

    if ($str === MARKER) {
        stream_header($src, $dst);
        $str = $src->reader->read_byte();
    }

    $dst->writer->write_exact($str);
    return true;
}

/**
 * @param BiDirectStream $src
 * @param BiDirectStream $dst
  */
function backward_byte($src, $dst) {
    $str = $src->reader->read_byte();
    $dst->writer->write_exact($str);
    return true;
}


/**
 * @param BiDirectStream $prev
 * @param BiDirectStream $next
  */
function stream_bytes($prev, $next) {
    warning("backward header from next to prev");
    print_stat($prev, $next);
    $header = $next->read_full_header();
    $prev->write_full_header($header);
    $prev->save_header($header);
    print_stat($prev, $next);


    warning("start stream");
    while(true) {
        if (!$prev->is_open()) {
            break;
        }
        $ok = forward_byte($prev, $next);
        if (!$prev->is_open() || !$ok) {
            break;
        }
        backward_byte($next, $prev);
    }
}


function process_query($incoming_stream) {
    $incoming = new BiDirectStream($incoming_stream, "Unknown");
    $header = $incoming->read_full_header();
    $destination = get_destination($header["forward_id"]);

    warning("destination is " . $destination);

    if ($destination === "echo") {
        warning("reach last component");
        $incoming->write_full_header($header);
        echo_bytes($incoming);
        return;
    }

    $out = component_open_stream($destination);
    if (is_null($out)) {
        warning("cannot open stream to " . $destination);
        return;
    }

    $outgoing = new BiDirectStream($out, $destination);
    $outgoing->write_full_header($header);
    $outgoing->save_header($header);
    stream_bytes($incoming, $outgoing);
}

function main() {
    while(true) {
        /** @var ComponentStream $incoming_stream */
        $incoming_stream = component_accept_stream();
        if (is_null($incoming_stream)) {

        }
        process_query($incoming_stream);
        component_finish_stream_processing($incoming_stream);
        warning("finish query process");
    }
}

// warning("Start component");

$scenario_query = component_client_send_query("proptest-generator", "");
$str = component_client_get_result($scenario_query);
$scenario = json_decode($str);
warning(var_export($scenario, true));
setup_die_timer();

main();
