#pragma once

template<class ArrayType, class IndexType>
IndexType dsu_get(ArrayType *arr, IndexType i) {
  if ((*arr)[i] == i) {
    return i;
  }
  return (*arr)[i] = dsu_get(arr, (*arr)[i]);
}

template<class ArrayType, class IndexType>
void dsu_uni(ArrayType *arr, IndexType i, IndexType j) {
  i = dsu_get(arr, i);
  j = dsu_get(arr, j);
  if (!(i == j)) {
    if (rand() & 1) {
      (*arr)[i] = j;
    } else {
      (*arr)[j] = i;
    }
  }
}
