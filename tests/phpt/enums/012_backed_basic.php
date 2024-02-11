@ok php8
<?php

enum HttpResponse : int {
    case Ok = 200;
    case NotFound = 404;
    case ServerError = 500;
}

foreach (HttpResponse::cases() as $v) {
    echo $v->name;
    echo $v->value;
}

$v = HttpResponse::Ok;
var_dump($v->value);
var_dump($v->name);

const c = HttpResponse::ServerError;
var_dump($v->value);
var_dump($v->name);
