@ok php8
<?php

enum Status : int {
    case Ok = 200;
    case NotFound = 404;
    case ServerError = 500;
}

var_dump(Status::from(200)->name);
var_dump(Status::from(404)->name);
var_dump(Status::from(500)->name);
