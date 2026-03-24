@kphp_runtime_should_not_warn ok
<?php

function should_warn() {
    warning("simple warning");
}

@should_warn();
