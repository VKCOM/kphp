@ok
<?php
function _array_(?array $v): ?array {
    return $v;
}

var_dump(_array_(null));
var_dump(_array_([]));

function _float_(?float $v): ?float {
    return $v;
}

var_dump(_float_(null));
var_dump(_float_(1.3));

function _int_(?int $v): ?int {
    return $v;
}

var_dump(_int_(null));
var_dump(_int_(1));

function _string_(?string $v): ?string {
    return $v;
}

var_dump(_string_(null));
var_dump(_string_("php"));

class simple_object{
  public function empty() {}
}

function _object_(?simple_object $v): ?simple_object {
  return $v;
}

if (!_object_(null)) {
  echo "Null returned\n";
}
if ($obj = _object_(new simple_object)) {
  echo get_class($obj), PHP_EOL;
}

