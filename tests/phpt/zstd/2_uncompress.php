@ok k2_skip
<?php

function test_uncompress() {
  var_dump(zstd_uncompress("foo bar baz"));
}

function test_uncompress_dict() {
  var_dump(zstd_uncompress_dict("foo bar baz", ""));
  var_dump(zstd_uncompress_dict("foo bar baz", "foo"));
  var_dump(zstd_uncompress_dict("foo bar baz", "foo bar baz"));
}

test_uncompress();
test_uncompress_dict();
