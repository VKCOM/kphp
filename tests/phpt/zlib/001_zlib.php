@ok
<?php

$x = "";
for ($i = 0; $i <= 100; $i++) {
  $x .= "abcdef";
}

$deflated = gzdeflate($x, 1);
$compressed = gzcompress($x, 1);
$encoded = gzencode($x, 1);

var_dump(crc32($deflated));
var_dump(crc32($compressed));
var_dump(crc32($encoded));

var_dump(gzinflate($deflated));
var_dump(gzuncompress($compressed));
var_dump(gzdecode($encoded));


function dump_test_deflate($ctx) {
    /**
     * @param string[] $deflated_chunks
     */
    $deflated_chunks = [];
    $chunks = ['myxa', 'myxophyta', 'myxopod', 'nab', 'nabbed', 'nabbing', 'nabit', 'nabk', 'nabob', 'nacarat', 'nacelle'];
    $out = "";
    for ($i = 0; $i < count($chunks); $i++) {
        $type = $i % 2 == 0 ? ZLIB_NO_FLUSH : ZLIB_SYNC_FLUSH;
        $deflated_chunks[] = deflate_add($ctx, $chunks[$i], $type);
        $out .= $deflated_chunks[$i];
    }
    $deflated_chunks[] = deflate_add($ctx, $chunks[count($chunks) - 1], ZLIB_FINISH);
    $out .= $deflated_chunks[count($deflated_chunks) - 1];
    var_dump(crc32($out));
    return $deflated_chunks;
}

function dump_test_inflate($ctx, $chunks) {
    $out = "";
    for ($i = 0; $i < count($chunks) - 1; $i++) {
        $type = $i % 2 == 0 ? ZLIB_NO_FLUSH : ZLIB_SYNC_FLUSH;
        $out .= inflate_add($ctx, $chunks[$i], $type);
    }
    $out .= inflate_add($ctx, $chunks[count($chunks) - 1], ZLIB_FINISH);
    var_dump(crc32($out));
    return $out;
}

$gzip_deflate_ctx = deflate_init(ZLIB_ENCODING_GZIP, ['window' => 10, 'memory' => 2]);
$raw_deflate_ctx = deflate_init(ZLIB_ENCODING_RAW, ['level' => 8]);
$deflate_ctx = deflate_init(ZLIB_ENCODING_DEFLATE, ['strategy' => ZLIB_HUFFMAN_ONLY]);

$encoded_chunks = dump_test_deflate($gzip_deflate_ctx);
$deflated_chunks = dump_test_deflate($raw_deflate_ctx);
$compressed_chunks = dump_test_deflate($deflate_ctx);

// var_dump(gzinflate($deflated));
// var_dump(gzuncompress($compressed));
// var_dump(gzdecode($encoded));

$gzip_inflate_ctx = inflate_init(ZLIB_ENCODING_GZIP, ['window' => 10, 'memory' => 2]);
$raw_inflate_ctx = inflate_init(ZLIB_ENCODING_RAW, ['level' => 8]);
$inflate_ctx = inflate_init(ZLIB_ENCODING_DEFLATE, ['strategy' => ZLIB_HUFFMAN_ONLY]);

$decoded = dump_test_inflate($gzip_inflate_ctx, $encoded_chunks);
$inflated = dump_test_inflate($raw_inflate_ctx, $deflated_chunks);
$uncompressed = dump_test_inflate($inflate_ctx, $compressed_chunks);

var_dump(gzdeflate($inflated));
var_dump(gzcompress($uncompressed));
var_dump(gzencode($decoded));
