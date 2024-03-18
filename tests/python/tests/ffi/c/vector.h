#define FFI_SCOPE "vector"
#define FFI_LIB "libvector.so"

double default_value;
const char *vectorlib_version;

struct Vector2 {
  double x;
  double y;
};

struct Vector2f {
  float x;
  float y;
};

typedef void (*Vector2WalkFunc)(int i, struct Vector2 *v);
typedef struct Vector2 (*Vector2MapFunc)(struct Vector2 v);
typedef double (*Vector2ReduceFunc)(struct Vector2 *v);

struct Vector2Array {
  struct Vector2 *data;
  int len;
};

struct Vector2fArray {
  struct Vector2f *data;
  int len;
};

void vector2_abs(struct Vector2 *vec);

void vector2f_abs(struct Vector2f *vec);

struct Vector2 *vector2_new_data(int size);
void vector2_data_free(struct Vector2 *data);

void vector2_array_fill(struct Vector2Array *array, ...);
void vector2_array_set(struct Vector2Array *array, int index, struct Vector2 val);
struct Vector2 vector2_array_get(struct Vector2Array *array, int index);
void vector2_array_init(struct Vector2Array *array, int size, ...);
void vector2_array_init_ptr(struct Vector2Array *array, int size, ...);
void vector2_array_alloc(struct Vector2Array *array, int size);
void vector2_array_free(struct Vector2Array *array);

void vector2_array_walk_ud(void *ud, struct Vector2Array *array, void (*f)(void *ud, int i, struct Vector2 *vec));
void vector2_array_walk(struct Vector2Array *array, Vector2WalkFunc f);
struct Vector2Array vector2_array_map(struct Vector2Array *array, Vector2MapFunc f);
double vector2_array_reduce(struct Vector2Array *array, Vector2ReduceFunc f);

void vector2f_array_free(struct Vector2fArray *array);
struct Vector2fArray vector2f_array_create(int size, void (*f)(struct Vector2f *v));
struct Vector2fArray vector2f_array_create_index(int size, void (*f)(int, struct Vector2f *));
struct Vector2f vector2f_array_get(struct Vector2fArray *array, int index);

int string_c_arg(int (*f)(const char *s));
