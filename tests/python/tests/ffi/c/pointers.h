#define FFI_SCOPE "pointers"
#define FFI_LIB "libpointers.so"

typedef struct SomeData {
  void *_opaque;
} SomeData;

typedef void *VoidPtr;

// Declaring with void argument on purpose.
SomeData *nullptr_data(void);

// Declaring with 2 unnamed arguments on purpose.
// See issue #424
void uint64_memset(uint64_t *, uint8_t);

uint64_t intptr_addr_value(int *ptr);
uint64_t voidptr_addr_value(void *ptr);

void write_int64(int64_t *dst, int64_t value);
int64_t read_int64(const int64_t *ptr);
int64_t read_int64_from_void(const void *ptr);
int64_t *read_int64p(int64_t **ptr);

uint8_t bytes_array_get(uint8_t *arr, int offset);
void bytes_array_set(uint8_t *arr, int offset, uint8_t val);

void *ptr_to_void(void *ptr);
const void *ptr_to_const_void(const void *ptr);
int void_strlen(const void *s);

void set_void_ptr(VoidPtr *dst, void *ptr);

void cstr_out_param(const char **out);

int strlen_safe(const char *s);

const char *nullptr_cstr();
const char *empty_cstr();

void set_unmanaged_data(void *);
void *get_unmanaged_data();

uint64_t alloc_callback_test(void *(*alloc_func)(int));

typedef void *(*AllocFunc)(int size);
uint64_t alloc_callback_test2(AllocFunc f);
