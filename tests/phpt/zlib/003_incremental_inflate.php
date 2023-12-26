@ok
<?php

function make_compressed_chunks($ctx) {
    $chunks = ['myxa', 'myxophyta', 'myxopod', 'nab', 'nabbed', 'nabbing', 'nabit', 'nabk', 'nabob', 'nacarat', 'nacelle'];
    $compressed_chunks = [];
    for($i = 0; $i < count($chunks) - 1; $i++) {
        $type = $i % 2 == 0 ? ZLIB_NO_FLUSH : ZLIB_SYNC_FLUSH;
        $compressed_chunks[] = deflate_add($ctx, $chunks[$i], $type);
    }
    $compressed_chunks[] = deflate_add($ctx, $chunks[count($chunks) - 1], ZLIB_FINISH);
    return $compressed_chunks;
}

function dump_test_inflate($ctx, $chunks) {
    $out = "";
    for ($i = 0; $i < count($chunks) - 1; $i++) {
        $type = $i % 2 == 0 ? ZLIB_NO_FLUSH : ZLIB_SYNC_FLUSH;
        $out .= inflate_add($ctx, $chunks[$i], $type);
    }
    $out .= inflate_add($ctx, $chunks[count($chunks) - 1], ZLIB_FINISH);
    return $out;
}

$gzip_compress_ctx = deflate_init(ZLIB_ENCODING_GZIP, ['window' => 10, 'memory' => 2]);
$raw_compress_ctx = deflate_init(ZLIB_ENCODING_RAW, ['level' => 8]);
$deflate_ctx = deflate_init(ZLIB_ENCODING_DEFLATE, ['strategy' => ZLIB_HUFFMAN_ONLY]);

$gzip_compressed_chunks = make_compressed_chunks($gzip_compress_ctx);
$raw_compressed_chunks = make_compressed_chunks($raw_compress_ctx);
$deflated_chunks = make_compressed_chunks($deflate_ctx);

$gzip_uncompress_ctx = inflate_init(ZLIB_ENCODING_GZIP, ['window' => 10, 'memory' => 2]);
$raw_uncompress_ctx = inflate_init(ZLIB_ENCODING_RAW, ['level' => 8]);
$inflate_ctx = inflate_init(ZLIB_ENCODING_DEFLATE, ['strategy' => ZLIB_HUFFMAN_ONLY]);

$gzip_uncompressed = dump_test_inflate($gzip_uncompress_ctx, $gzip_compressed_chunks);
$raw_uncompressed = dump_test_inflate($raw_uncompress_ctx, $raw_compressed_chunks);
$inflated = dump_test_inflate($inflate_ctx, $deflated_chunks);

var_dump(crc32($gzip_uncompressed));
var_dump(crc32($raw_uncompressed));
var_dump(crc32($inflated));

var_dump(gzencode($gzip_uncompressed));
var_dump(gzcompress($raw_uncompressed));
var_dump(gzdeflate($inflated));
