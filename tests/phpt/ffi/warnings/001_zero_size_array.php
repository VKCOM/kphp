@kphp_runtime_should_warn k2_skip
/zero array size/
<?

// At some point we discussed whether zero sized arrays should be allowed.
// Although it seems doable and convenient not to care about the size value,
// it doesn't work in PHP ("Cannot instantiate FFI\CData of zero size" error).
// So we give an error in KPHP as well.

$size = 0;
$_ = FFI::new("int[$size]");
