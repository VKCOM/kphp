@ok
<?php

function testGzcompressAndGzuncompress() {
    $originalData = "This is a test string to be compressed and decompressed.";

    // Compress the data
    $compressedData = gzcompress($originalData);
    if ($compressedData === false) {
        echo "Compression failed.\n";
        return;
    }

    // Decompress the data
    $decompressedData = gzuncompress($compressedData);
    if ($decompressedData === false) {
        echo "Decompression failed.\n";
        return;
    }

    if ($originalData === $decompressedData) {
        echo "Test passed: Decompressed data matches the original.\n";
    } else {
        echo "Test failed: Decompressed data does not match the original.\n";
    }
}

function testGzcompressWithEmptyString() {
    $originalData = "";

    // Compress the data
    $compressedData = gzcompress($originalData);
    if ($compressedData === false) {
        echo "Compression failed for empty string.\n";
        return;
    }

    // Decompress the data
    $decompressedData = gzuncompress($compressedData);
    if ($decompressedData === false) {
        echo "Decompression failed for empty string.\n";
        return;
    }

    if ($originalData === $decompressedData) {
        echo "Test passed: Decompressed empty string matches the original.\n";
    } else {
        echo "Test failed: Decompressed empty string does not match the original.\n";
    }
}

// FIXME: KPHP's implementation differs from K2's and PHP's ones. It should return false on errors instead of an empty string
// function testGzuncompressWithInvalidData() {
//     $invalidData = "This is not compressed data.";
//
//     // Attempt to decompress invalid data
//     $decompressedData = @gzuncompress($invalidData);
//     if ($decompressedData === false) {
//         echo "Test passed: Decompression failed for invalid data as expected.\n";
//     } else {
//         echo "Test failed: Decompression should fail for invalid data.\n";
//     }
// }

testGzcompressAndGzuncompress();
testGzcompressWithEmptyString();
// testGzuncompressWithInvalidData();
