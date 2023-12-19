@ok
<?php

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

$gzip_ctx = inflate_init(ZLIB_ENCODING_GZIP, ['window' => 10, 'memory' => 2]);
$raw_ctx = inflate_init(ZLIB_ENCODING_RAW, ['level' => 8]);
$inflate_ctx = inflate_init(ZLIB_ENCODING_DEFLATE, ['strategy' => ZLIB_HUFFMAN_ONLY]);

$decoded = dump_test_inflate($gzip_ctx, $encoded_chunks);
$inflated = dump_test_inflate($raw_ctx, $deflated_chunks);
$uncompressed = dump_test_inflate($inflate_ctx, $compressed_chunks);

var_dump(gzdeflate($inflated));
var_dump(gzcompress($uncompressed));
var_dump(gzencode($decoded));
