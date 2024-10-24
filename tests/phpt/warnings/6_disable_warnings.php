@kphp_runtime_should_not_warn ok k2_skip
<?php

function should_warn() {
    warning("simple warning");
}

@should_warn();
