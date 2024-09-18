@ok
<?php

echo "Zero ";
header_register_callback(function () {
    throw new Exception('Exception');
});
echo "One ";
