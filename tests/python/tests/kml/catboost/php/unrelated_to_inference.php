<?php

function ensure($x) {
    if (!$x) {
        die(1);
    }
}

function main() {
    $model_name = "catboost_tiny_1float_1hot_10trees";

    ensure(kml_model_exist($model_name));
    ensure(["x", "y"] === kml_feature_names($model_name));
    ensure("val_1" == kml_custom_property($model_name, "property_1"));
    ensure("val_2, val_3" === kml_custom_property($model_name, "property_2"));
}

main()