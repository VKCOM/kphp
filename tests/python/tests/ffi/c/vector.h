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

struct Vector2Array {
  struct Vector2 *data;
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
