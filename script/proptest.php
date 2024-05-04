<?php
define('MARKER', "\0");

class StreamHolder {
    /** @var ComponentStream $stream*/
    public $stream;

    /** @param ComponentStream $s*/
    public function __construct($s) {
        $this->stream = $s;
    }
}

class Reader extends StreamHolder
{

    public function __construct($stream) {
        parent::__construct($stream);
    }

    public function read_header() {
        warning("read header call");
        $result = component_stream_read_exact($this->stream, 3);
        if (strlen($result) != 3) {
            critical_error("cannot read header");
        }
        debug_print_string($result);
        $marker = $result[0];
        $forward_id = $result[1];
        $seed = $result[2];
        warning("get seed " .$seed);
        if ($marker !== MARKER) {
            critical_error("marker non zero");
        }
        return shape(["forward_id" => $forward_id, "seed" => $seed]);
    }

    public function is_closed() {
        return $this->stream->is_read_closed();
    }

    public function read_byte() {
        return component_stream_read_exact($this->stream, 1);
        //return component_stream_read_nonblock($this->stream);
    }
}

class Writer extends StreamHolder
{

    public function __construct($stream)
    {
        parent::__construct($stream);
    }

    public function write_header($previous_header)
    {
        debug_print_string($previous_header["forward_id"]);
        debug_print_string($previous_header["seed"]);
        $str = "";
        $str .= MARKER;
        $str .= increment_byte($previous_header["forward_id"]);
        $str .= $previous_header["seed"];
        debug_print_string($str);
        $write = component_stream_write_exact($this->stream, $str);
        if ($write != 3) {
            critical_error("cannot write header");
        }
    }

    public function is_closed()
    {
        return $this->stream->is_write_closed();
    }

    public function write_exact($message) {
        return component_stream_write_exact($this->stream, $message);
    }
}

function get_destination($forward_id)
{
    global $scenario;
    $len = count($scenario["forward"]["num"]);
    for ($i = 0; $i < $len; $i++) {
        if ($scenario["forward"]["num"][$i] === $forward_id) {
            return $scenario["forward"]["names"][$i];
        }
    }
    warning("cannot find destination");
    return "";
}

/**
 * @param Reader $input
 * @param Writer $output
  */
function forward_bytes($input, $output) {
    $count = 0;
    while (!$input->is_closed() && !$output->is_closed()) {
        testyield();
        $str = $input->read_byte();
        $output->write_exact($str);
        $count++;
        warning("forward ". $count . " bytes");
    }
}

function process_query($incoming_stream) {
    $reader_in = new Reader($incoming_stream);
    $header = $reader_in->read_header();
    $destination = get_destination($header["forward_id"] + 1); // why here + 1?
    warning("destination is " . $destination);
    if ($destination === "echo") {
        $writer_out = new Writer($incoming_stream);
        $writer_out->write_header($header);
        forward_bytes($reader_in, $writer_out);
        return;
    }

    $outgoing_stream = component_open_stream($destination);
    if (is_null($outgoing_stream)) {
        warning("cannot open stream to " . $destination);
        return;
    }

    $writer_out = new Writer($outgoing_stream);
    $writer_out->write_header($header);
    forward_bytes($reader_in, $writer_out);

    $reader_out = new Writer($incoming_stream);
    $writer_in = new Reader($outgoing_stream);
    forward_bytes($writer_in, $reader_out);
}

function main() {
    while (true) {
        /** @var ComponentStream $incoming_stream */
        $incoming_stream = component_accept_stream();
        if (is_null($incoming_stream)) {
        }
        process_query($incoming_stream);
        component_finish_stream_processing($incoming_stream);
        warning("finish query process");
    }
}



$scenario_query = component_client_send_query("proptest-generator", "");
$str = component_client_get_result($scenario_query);
$scenario = json_decode($str);
warning(var_export($scenario, true));

main();
