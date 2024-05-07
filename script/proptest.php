<?php
define('MARKER', "\0");

class StreamHolder {
    /** @var ComponentStream $stream*/
    public $stream;

    public $name;

    /** @param ComponentStream $s*/
    public function __construct($s, $n) {
        $this->stream = $s;
        $this->name = $n;
    }

    public function close_stream() {
        component_close_stream($this->stream);
    }

    public function reopen() {
        $this->close_stream();
        $this->stream = component_open_stream($this->name);
        if (is_null($this->stream)) {
            warning("cannot open stream to " . $this->$name);
            return false;
        }
        return true;
    }
}

class Reader {
    /** @var ComponentStream $stream*/
    public $stream;

    /** @param ComponentStream $s*/
    public function __construct($s) {
        $this->stream = $s;
    }

    public function is_read_closed() {
        return $this->stream->is_read_closed();
    }
}

class Writer {
    /** @var ComponentStream $stream*/
    public $stream;

    /** @param ComponentStream $s*/
    public function __construct($s) {
        $this->stream = $s;
    }
}

class BiDirectStream extends StreamHolder {
    public function __construct($stream, $name) {
        parent::__construct($stream, $name);
    }

    public function read_header() {
        warning("read header call");
        $result = component_stream_read_exact($this->stream, 3);
        if (strlen($result) != 3) {
            critical_error("cannot read header");
        }

        $forward_id = byte_to_int($result[1]);
        $seed = $result[2];
        warning("get forward_id " . $forward_id);
        return shape(["forward_id" => $forward_id, "seed" => $seed]);
    }

    public function is_read_closed() {
        return $this->stream->is_read_closed();
    }

    public function read_byte() {
        return component_stream_read_exact($this->stream, 1);
    }

    public function write_header($previous_header)
    {
        $str = "";
        $str .= MARKER;
        $str .= int_to_byte($previous_header["forward_id"] + 1);
        $str .= $previous_header["seed"];
        debug_print_string($str);
        $write = component_stream_write_exact($this->stream, $str);
        if ($write != 3) {
            critical_error("cannot write header");
        }
    }

    public function is_write_closed()
    {
        return $this->stream->is_write_closed();
    }

    public function write_exact($message) {
        return component_stream_write_exact($this->stream, $message);
    }

    public function is_open()
    {
        return !$this->is_write_closed() && !$this->is_read_closed();
    }

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
//     for ($i = 0; $i < $len; $i++) {
//         if ($scenario["forward"]["num"][$] === $forward_id) {
//             return $scenario["forward"]["names"][$i];
//         }
//     }
//     warning("cannot find destination for forward_id " . $forward_id );
//     return "";
}


/**
 * @param BiDirectStream $src
 * @param BiDirectStream $dst
 */
function forward_byte($src, $dst) {
    warning("forward_byte");
    $str = $src->read_byte();
    if ($dst->is_write_closed()) {
        $successful = $dst->reopen();
        if (!$successful) {
            return false;
        }
    }
    if ($str === MARKER) {
        warning("forward marker");
        $str .= $src->read_byte();
        $str .= $src->read_byte();
    }
    $dst->write_exact($str);
    return true;
}

/**
 * @param BiDirectStream $src
 * @param BiDirectStream $dst
  */
function backward_byte($src, $dst) {
    warning("backward_byte");
    $str = $src->read_byte();
    if ($str === MARKER) {
        warning("backward marker");
        $str .= $src->read_byte();
        $str .= $src->read_byte();
    }
    $dst->write_exact($str);
    return true;
}

/**
 * @param BiDirectStream $stream
  */
function echo_bytes($stream) {
    warning("echo_bytes");
    while(!$stream->is_read_closed() && !$stream->is_write_closed()) {
        backward_byte($stream, $stream);
    }
}

/**
 * @param BiDirectStream $prev
 * @param BiDirectStream $next
  */
function stream_bytes($prev, $next) {
    // forward header from next to prev
    warning("backward_byte header");
    backward_byte($next, $prev);


    warning("forward_bytes body");
    while($prev->is_open()) {
        $ok = forward_byte($prev, $next);
        if (!$ok) {
            break;
        }
        backward_byte($next, $prev);
    }
}

function process_query($incoming_stream) {
    $incoming = new BiDirectStream($incoming_stream, "Unknown");
    $header = $incoming->read_header();
    $destination = get_destination($header["forward_id"]);
    warning("destination is " . $destination);

    if ($destination === "echo") {
        warning("reach last component");
        $incoming->write_header($header);
        echo_bytes($incoming);
        return;
    }

    $out = component_open_stream($destination);
    if (is_null($out)) {
        warning("cannot open stream to " . $destination);
        return;
    }

    $outgoing = new BiDirectStream($out, $destination);
    $outgoing->write_header($header);
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
