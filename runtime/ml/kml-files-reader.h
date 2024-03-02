#pragma once

// TODO
// Maybe kml-reader / kml-writer as lib?
// Just to be synchronized with standalone app

#include "internals.h"

ml_internals::MLModel kml_file_read(const char *kml_filename);
