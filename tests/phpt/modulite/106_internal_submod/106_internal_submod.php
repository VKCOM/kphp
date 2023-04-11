@kphp_should_fail
/Failed loading Messages106\/.modulite.yaml:/
/can't require @api\/impl: @api\/impl is internal in @api/
/can't require @msg\/channels\/infra: @msg\/channels\/infra is internal in @msg\/channels/
/Failed loading API106\/Impl\/.modulite.yaml:/
/can't require @msg\/core: @msg\/core is internal in @msg/
/Failed loading parent\/morechild1\/.modulite.yaml:/
/can't require @parent\/child1\/child2\/child3my: @parent\/child1\/child2\/child3my is internal in @parent\/child1\/child2/
/can't require @parent\/child1\/child2my: @parent\/child1\/child2my is internal in @parent\/child1/
<?php

Messages106\Messages106::act();

require_once __DIR__ . '/parent/ParentFuncs_106.php';
ParentFuncs_106::testThatCantAccessSubsubchild();
