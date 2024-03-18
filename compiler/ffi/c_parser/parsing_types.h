// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

struct FFIType;

namespace ffi {

struct TypeSpecifier {
  FFIType *type;
};
struct DeclarationSpecifiers {
  FFIType *type;
};
struct InitDeclaratorList {
  FFIType *type;
};
struct AbstractDeclarator {
  FFIType *type;
};
struct DirectAbstractDeclarator {
  FFIType *type;
};
struct DirectDeclarator {
  FFIType *type;
};
struct Declarator {
  FFIType *type;
};
struct TypeQualifier {
  FFIType *type;
};
struct Pointer {
  FFIType *type;
};
struct SpecifierQualifierList {
  FFIType *type;
};

} // namespace ffi
