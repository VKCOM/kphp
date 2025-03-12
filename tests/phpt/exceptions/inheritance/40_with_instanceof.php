@ok
<?php

class ApiException extends Exception {
    function __construct() { parent::__construct(__CLASS__, 111); }
    function more() { echo "ApiException more\n"; }
}

function demo() {
    try {
        if (1) throw new ApiException();
    } catch(Exception $ex) {
        if ($ex instanceof ApiException) {
            $ex->more();
        }
        echo $ex->getCode(), "\n";
    }
}

demo();
