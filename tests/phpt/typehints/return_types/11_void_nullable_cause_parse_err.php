@kphp_should_fail
/Type Error: mixing void and non-void expressions/
<?php
function foo(): ?void {
}
foo();

