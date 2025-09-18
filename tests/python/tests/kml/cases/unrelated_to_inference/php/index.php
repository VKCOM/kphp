<?php

function ensure($x)
{
    if (!$x) {
        header('HTTP/1.1 500 KML assertion failed', true, 500);
        die(1);
    }
}

function main() {
    $model_name = "catboost_tiny_1float_1hot_10trees";

    ensure(kml_model_exists($model_name));
    ensure(in_array($model_name, kml_available_models()));
    ensure(count(kml_available_models()) == 2); // 2 models in models/catboost
    ensure(["x", "y"] === kml_get_feature_names($model_name));
    ensure("val_1" == kml_get_custom_property($model_name, "property_1"));
    ensure("val_2, val_3" === kml_get_custom_property($model_name, "property_2"));
    ensure(null === kml_get_custom_property($model_name, "property_absent"));
    ensure(null === kml_xgboost_predict_matrix("absent_model", []));
}

main();
