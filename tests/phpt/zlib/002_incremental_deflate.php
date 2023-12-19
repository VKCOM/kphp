@ok
<?php

function dump_test_deflate($ctx) {
    $chunks = ['myxa', 'myxophyta', 'myxopod', 'nab', 'nabbed', 'nabbing', 'nabit', 'nabk', 'nabob', 'nacarat', 'nacelle'];
    $out = "";
    for($i = 0; $i < count($chunks); $i++) {
        $type = $i % 2 == 0 ? ZLIB_NO_FLUSH : ZLIB_SYNC_FLUSH;
        $out .= deflate_add($ctx, $chunks[$i], $type);
    }
    $out .= deflate_add($ctx, $chunks[count($chunks) - 1], ZLIB_FINISH);
    return $out;
}

$gzip_ctx = deflate_init(ZLIB_ENCODING_GZIP, ['window' => 10, 'memory' => 2]);
$raw_ctx = deflate_init(ZLIB_ENCODING_RAW, ['level' => 8]);
$deflate_ctx = deflate_init(ZLIB_ENCODING_DEFLATE, ['strategy' => ZLIB_HUFFMAN_ONLY]);

$encoded = dump_test_deflate($gzip_ctx);
$deflated = dump_test_deflate($raw_ctx);
$compressed = dump_test_deflate($deflate_ctx);

var_dump(crc32($deflated));
var_dump(crc32($compressed));
var_dump(crc32($encoded));

var_dump(gzinflate($deflated));
var_dump(gzuncompress($compressed));
var_dump(gzdecode($encoded));
