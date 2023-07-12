@ok

try {
    $callable = fn() => throw new Exception("err");
    $callable();
} catch (Exception $err) {
    echo $err->getMessage();
}
