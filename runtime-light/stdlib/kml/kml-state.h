// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/stdlib/kml/inference-context.h"
#include "runtime-common/stdlib/kml/models-context.h"
#include "runtime-light/stdlib/kml/kml-file-api.h"

using KmlComponentState = KmlModelsContext<kphp::kml::detail::dir_traverser<kphp::memory::script_allocator>, kphp::memory::script_allocator>;

using KmlInstanceState = KmlInferenceContext<kphp::memory::script_allocator>;
