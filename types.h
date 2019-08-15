/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

#ifndef TYPES_H_
#define TYPES_H_

#ifdef USE_TYPES_GENERATED
#include "types-generated.h"
#else /* USE_TYPES_GENERATED */
#include "types-handcraft.h"
#endif /* USE_TYPES_GENERATED */

/*
 * First-class data in JavaScript is represented as a JSValue.
 * JSValue has 64 bits, where least sifnificat three bits is its tag.
 *
 *  ---------------------------------------------------
 *  |  pointer / immediate value                  |tag|
 *  ---------------------------------------------------
 *  63                                             210
 */
#define TAGOFFSET (3)
#define TAGMASK   (0x7)  /* 111 */

#define get_tag(p)      (((Tag)(p)) & TAGMASK)
#define put_tag(p,t)    ((JSValue)((uint64_t)(p) + (t)))
#define clear_tag(p)    ((uint64_t)(p) & ~TAGMASK)
#define remove_tag(p,t) (clear_tag(p))
#define equal_tag(p,t)  (get_tag((p)) == (t))

/*
 * Pair of two pointer tags
 * Note that the result of TAG_PAIR is of type Tag
 */
#define TAG_PAIR(t1, t2) ((t1) | ((t2) << TAGOFFSET))

#define TP_OBJOBJ TAG_PAIR(T_GENERIC, T_GENERIC)
#define TP_OBJSTR TAG_PAIR(T_GENERIC, T_STRING)
#define TP_OBJFLO TAG_PAIR(T_GENERIC, T_FLONUM)
#define TP_OBJSPE TAG_PAIR(T_GENERIC, T_SPECIAL)
#define TP_OBJFIX TAG_PAIR(T_GENERIC, T_FIXNUM)
#define TP_STROBJ TAG_PAIR(T_STRING, T_GENERIC)
#define TP_STRSTR TAG_PAIR(T_STRING, T_STRING)
#define TP_STRFLO TAG_PAIR(T_STRING, T_FLONUM)
#define TP_STRSPE TAG_PAIR(T_STRING, T_SPECIAL)
#define TP_STRFIX TAG_PAIR(T_STRING, T_FIXNUM)
#define TP_FLOOBJ TAG_PAIR(T_FLONUM, T_GENERIC)
#define TP_FLOSTR TAG_PAIR(T_FLONUM, T_STRING)
#define TP_FLOFLO TAG_PAIR(T_FLONUM, T_FLONUM)
#define TP_FLOSPE TAG_PAIR(T_FLONUM, T_SPECIAL)
#define TP_FLOFIX TAG_PAIR(T_FLONUM, T_FIXNUM)
#define TP_SPEOBJ TAG_PAIR(T_SPECIAL, T_GENERIC)
#define TP_SPESTR TAG_PAIR(T_SPECIAL, T_STRING)
#define TP_SPEFLO TAG_PAIR(T_SPECIAL, T_FLONUM)
#define TP_SPESPE TAG_PAIR(T_SPECIAL, T_SPECIAL)
#define TP_SPEFIX TAG_PAIR(T_SPECIAL, T_FIXNUM)
#define TP_FIXOBJ TAG_PAIR(T_FIXNUM, T_GENERIC)
#define TP_FIXSTR TAG_PAIR(T_FIXNUM, T_STRING)
#define TP_FIXFLO TAG_PAIR(T_FIXNUM, T_FLONUM)
#define TP_FIXSPE TAG_PAIR(T_FIXNUM, T_SPECIAL)
#define TP_FIXFIX TAG_PAIR(T_FIXNUM, T_FIXNUM)

typedef uint16_t Register;
typedef int16_t  Displacement;
typedef uint16_t Subscript;
typedef uint16_t Tag;

/*
 * Hidden Class Transition
 *
 * A hidden class has a hash table, where each key is a property name
 * represented in a JS string.
 * Associated value for a key is either an index in the property array
 * (as a fixnum) or a pointer to the next hidden class.
 * The former is called `index entry' and the latter is called `transition
 * entry'.
 * Member n_entries has the number of entries registered in the map, i.e.,
 * the sum of the number of index entries and that of the transition entries.
 */
typedef struct hidden_class {
  uint32_t n_entries;
  uint32_t htype;            /* HTYPE_TRANSIT or HTYPE_GROW */
  HashTable *map;            /* map which is explained above */
  uint32_t n_props;          /* number of properties */
  uint32_t n_limit_props;    /* capacity of the array for properties */
  uint32_t n_embedded_props; /* number of properites embedded in the object */
  uint32_t n_special_props;  /* number of properties that are not registered
                              * in the map, which may not be JSValues */
  JSValue __proto__;         /* Hidden class or JS_EMPTY, which means
                              * object instance has __proto__ property */

#ifdef HC_DEBUG
  struct hidden_class *dbg_prev;
#endif /* HC_DEBUG */
#ifdef PROFILE
  struct hidden_class *prev;
  uint32_t n_profile_enter;  /* number of times this class is used for object
                              * marked for profilling */
#endif /* PROFILE */
  uint32_t n_enter;          /* number of times this class is used */
  uint32_t n_exit;           /* number of times this class is left */
} HiddenClass;

#define hidden_n_entries(h)        ((h)->n_entries)
#define hidden_htype(h)            ((h)->htype)
#define hidden_n_enter(h)          ((h)->n_enter)
#define hidden_n_exit(h)           ((h)->n_exit)
#define hidden_map(h)              ((h)->map)
#define hidden_n_props(h)          ((h)->n_props)
#define hidden_n_limit_props(h)    ((h)->n_limit_props)
#define hidden_n_embedded_props(h) ((h)->n_embedded_props)
#define hidden_n_special_props(h)  ((h)->n_special_props)
#define hidden_proto(h)            ((h)->__proto__)
#ifdef HC_DEBUG
#define hidden_dbg_prev(h)         ((h)->dbg_prev)
#endif /* HC_DEBUG */
#ifdef PROFILE
#define hidden_prev(h)             ((h)->prev)
#define hidden_n_profile_enter(h)  ((h)->n_profile_enter)
#endif /* PROFILE */

#define HTYPE_TRANSIT   0
#define HTYPE_GROW      1

#define PSIZE_NORMAL   1  /* default initial size of the property array */
#define PSIZE_BIG    100
#define PSIZE_DELTA    1  /* delta when expanding the property array */
#define PSIZE_LIMIT  500  /* limit size of the property array */
#define HSIZE_NORMAL   1  /* default initial size of the map (hash table) */
#define HSIZE_BIG    100

#define CHECK_INCREASE_PROPERTY(n) do {         \
    if ((n) + PSIZE_DELTA > PSIZE_LIMIT)        \
      LOG_EXIT("too many properties");          \
  } while(0)

/*
 * Object
 * tag == T_GENERIC
 */
typedef struct object_cell {
#ifdef PROFILE
  int profile_id;
#endif /* PROFILE */
  HiddenClass *klass;     /* Hidden class for this object */
  JSValue eprop[PSIZE_NORMAL];
} Object;

#define remove_simple_object_tag remove_normal_simple_object_tag
#define put_simple_object_tag    put_normal_simple_object_tag

#define allocate_simple_object(ctx, nemb)               \
  allocate_jsobject((ctx), (nemb), HTAG_SIMPLE_OBJECT)
#define make_simple_object(ctx, n)                              \
  (put_simple_object_tag(allocate_simple_object(ctx, (n))))
#define remove_object_tag(p)    ((Object *)clear_tag(p))

#define obj_hidden_class(p)    ((remove_object_tag(p))->klass)
#define obj_hidden_class_map(p) (hidden_map(obj_hidden_class(p)))
#define obj_eprop(p)           ((remove_object_tag(p))->eprop)
#define obj_ovf_props(p,hc)    (obj_eprop(p)[hidden_n_embedded_props(hc) - 1])
static inline JSValue get_obj_prop_index(JSValue p, int index)
{
  HiddenClass *hc = obj_hidden_class(p);
  int n_embedded = hidden_n_embedded_props(hc);
  if (hidden_n_limit_props(hc) == 0 || index < n_embedded - 1)
    return obj_eprop(p)[index];
  else {
    JSValue *of = (JSValue *) obj_ovf_props(p, hc);
    return of[index - (n_embedded - 1)];
  }
}
static inline void set_obj_prop_index(JSValue p, int index, JSValue v)
{
  HiddenClass *hc = obj_hidden_class(p);
  int n_embedded = hidden_n_embedded_props(hc);
  if (hidden_n_limit_props(hc) == 0 || index < n_embedded - 1)
    obj_eprop(p)[index] = v;
  else {
    JSValue *of = (JSValue *) obj_ovf_props(p, hc);
    of[index - (n_embedded - 1)] = v;
  }
}
#ifdef PROFILE
#define obj_profile_id(p)      ((remove_object_tag(p))->profile_id)
#endif /* PROFILE */

#define obj_header_tag(x)      gc_obj_header_type(remove_object_tag(x))
#define is_obj_header_tag(o,t) (is_object((o)) && (obj_header_tag((o)) == (t)))

#define HHH 0

#define new_normal_object(ctx)      new_simple_object(ctx)
#define new_normal_iterator(ctx, o) new_iterator(ctx, o)

/*
 * Array
 */

#define ARRAY_SPECIAL_PROPS       3
#define ARRAY_XPROP_INDEX_SIZE    0
#define ARRAY_XPROP_INDEX_LENGTH  1
#define ARRAY_XPROP_INDEX_BODY    2
#define ARRAY_NORMAL_PROPS        1
#define ARRAY_PROP_INDEX_LENGTH   3
#define ARRAY_EMBEDDED_PROPS     (ARRAY_SPECIAL_PROPS + ARRAY_NORMAL_PROPS)

#undef remove_normal_array_tag
#define remove_normal_array_tag(p) ((Object *)remove_tag((p), T_GENERIC))

#define allocate_array(ctx)                                     \
  (allocate_jsobject((ctx), ARRAY_EMBEDDED_PROPS, HTAG_ARRAY))
#define make_array(ctx)                         \
  (put_normal_array_tag(allocate_array(ctx)))

#define array_object_p(a)        (remove_normal_array_tag(a))
#define array_eprop(a,t,i)                              \
  (*(t*)&(remove_normal_array_tag(a))->eprop[i])
#define array_size(a)   array_eprop(a, uint64_t, ARRAY_XPROP_INDEX_SIZE)
#define array_length(a) array_eprop(a, uint64_t, ARRAY_XPROP_INDEX_LENGTH)
#define array_body(a)   array_eprop(a, JSValue *, ARRAY_XPROP_INDEX_BODY)
#define array_body_index(a,i)                   \
  ((JSValue *)array_body(a))[(i)]

#define ASIZE_INIT   10       /* default initial size of the C array */
#define ASIZE_DELTA  10       /* delta when expanding the C array */
#define ASIZE_LIMIT  100      /* limit size of the C array */
#define MAX_ARRAY_LENGTH  ((uint64_t)(0xffffffff))

#define increase_asize(n)     (((n) >= ASIZE_LIMIT)? (n): ((n) + ASIZE_DELTA))

#define MINIMUM_ARRAY_SIZE  100


/*
 * Function
 */

#define allocate_function(ctx)                                  \
  allocate_jsobject((ctx), FUNC_EMBEDDED_PROPS, HTAG_FUNCTION)
#define make_function(ctx)   (put_normal_function_tag(allocate_function(ctx)))

#define FUNC_SPECIAL_PROPS        2
#define FUNC_XPROP_INDEX_FTENTRY  0
#define FUNC_XPROP_INDEX_ENV      1
#define FUNC_NORMAL_PROPS         1
#define FUNC_PROP_INDEX_PROTOTYPE 2
#define FUNC_EMBEDDED_PROPS       (FUNC_SPECIAL_PROPS + FUNC_NORMAL_PROPS)

#undef remove_normal_function_tag
#define remove_normal_function_tag(p)  ((Object *)remove_tag((p), T_GENERIC))

#define func_object_p(o)    (remove_normal_function_tag(o))
#define func_eprop(o,t,i)                               \
  (*(t*)&(remove_normal_function_tag(o))->eprop[i])
#define func_table_entry(o)                                     \
  func_eprop(o, FunctionTable *, FUNC_XPROP_INDEX_FTENTRY)
#define func_environment(o)                             \
  func_eprop(o, FunctionFrame *, FUNC_XPROP_INDEX_ENV)


/*
 * Builtin
 */

typedef void (*builtin_function_t)(Context*, int, int);

#define BUILTIN_SPECIAL_PROPS        3
#define BUILTIN_XPROP_INDEX_BODY     0
#define BUILTIN_XPROP_INDEX_CONSTR   1
#define BUILTIN_XPROP_INDEX_ARGC     2
#define BUILTIN_NORMAL_PROPS         1
#define BUILTIN_PROP_INDEX_PROTOTYPE 3
#define BUILTIN_EMBEDDED_PROPS (BUILTIN_SPECIAL_PROPS + BUILTIN_NORMAL_PROPS)

#undef remove_normal_builtin_tag
#define remove_normal_builtin_tag(p)  ((Object *)remove_tag((p), T_GENERIC))

#define allocate_builtin(ctx)                                           \
  allocate_jsobject((ctx), BUILTIN_EMBEDDED_PROPS, HTAG_BUILTIN)
#define make_builtin(ctx)   (put_normal_builtin_tag(allocate_builtin(ctx)))

#define builtin_object_p(o)          (remove_normal_builtin_tag(o))
#define builtin_eprop(o,t,i)                            \
  (*(t*)&(remove_normal_builtin_tag(o))->eprop[i])
#define builtin_body(o)                                         \
  builtin_eprop(o, builtin_function_t, BUILTIN_XPROP_INDEX_BODY)
#define builtin_constructor(o)                          \
  builtin_eprop(o, builtin_function_t, BUILTIN_XPROP_INDEX_CONSTR)
#define builtin_n_args(o)                               \
  builtin_eprop(o, uint64_t, BUILTIN_XPROP_INDEX_ARGC)


#ifdef USE_REGEXP
/*
 * Regexp
 */

#include <oniguruma.h>

#define F_REGEXP_NONE      (0x0)
#define F_REGEXP_GLOBAL    (0x1)
#define F_REGEXP_IGNORE    (0x2)
#define F_REGEXP_MULTILINE (0x4)

#define REX_SPECIAL_PROPS       6
#define REX_XPROP_INDEX_PATTERN 0
#define REX_XPROP_INDEX_REX     1
#define REX_XPROP_INDEX_GFLAG   2
#define REX_XPROP_INDEX_IFLAG   3
#define REX_XPROP_INDEX_MFLAG   4
#define REX_XPROP_INDEX_LASTIDX 5
#define REX_NORMAL_PROPS        1
#define REX_EMBEDDED_PROPS      (REX_SPECIAL_PROPS + REX_NORMAL_PROPS)

#undef remove_normal_regexp_tag
#define remove_normal_regexp_tag(p)  ((Object *)remove_tag((p), T_GENERIC))

#define allocate_regexp(ctx)                                    \
  allocate_jsobject((ctx), REX_EMBEDDED_PROPS, HTAG_REGEXP)
#define make_regexp(ctx)     (put_normal_regexp_tag(allocate_regexp(ctx)))

#define regexp_object_p(o)   (remove_normal_regexp_tag(o))
#define regexp_eprop(o,t,i)                             \
  (*(t*)&(remove_normal_regexp_tag(o))->eprop[i])
#define regexp_pattern(o)    regexp_eprop(o, char*,    REX_XPROP_INDEX_PATTERN)
#define regexp_reg(o)        regexp_eprop(o, regex_t*, REX_XPROP_INDEX_REX)
#define regexp_global(o)     regexp_eprop(o, int,      REX_XPROP_INDEX_GFLAG)
#define regexp_ignorecase(o) regexp_eprop(o, int,      REX_XPROP_INDEX_IFLAG)
#define regexp_multiline(o)  regexp_eprop(o, int,      REX_XPROP_INDEX_MFLAG)
#define regexp_lastindex(o)  regexp_eprop(o, int,      REX_XPROP_INDEX_LASTIDX)

#endif /* USE_REGEXP */

/*
 * Boxed values
 */
#define BOXED_SPECIAL_PROPS     1
#define BOXED_XPROP_INDEX_VALUE 0
#define BOXED_NORMAL_PROPS      1
#define BOXED_EMBEDDED_PROPS    (BOXED_SPECIAL_PROPS + BOXED_NORMAL_PROPS)

#define BOXEDSTR_NORMAL_PROPS      1
#define BOXEDSTR_PROP_INDEX_LENGTH 1
#define BOXEDSTR_EMBEDDED_PROPS  (BOXED_SPECIAL_PROPS + BOXEDSTR_NORMAL_PROPS)

#undef remove_normal_number_object_tag
#define remove_normal_number_object_tag(p)      \
  ((Object *)remove_tag((p), T_GENERIC))
#undef remove_normal_string_object_tag
#define remove_normal_string_object_tag(p)      \
  ((Object *)remove_tag((p), T_GENERIC))
#undef remove_normal_boolean_object_tag
#define remove_normal_boolean_object_tag(p)      \
  ((Object *)remove_tag((p), T_GENERIC))

#define make_number_object(ctx)                                         \
  put_normal_number_object_tag(allocate_jsobject((ctx), BOXED_EMBEDDED_PROPS, \
                                                 HTAG_BOXED_NUMBER))
#define make_string_object(ctx)                                         \
  put_normal_string_object_tag(allocate_jsobject((ctx),                 \
                                                 BOXEDSTR_EMBEDDED_PROPS, \
                                                 HTAG_BOXED_STRING))
#define make_boolean_object(ctx)                                         \
  put_normal_boolean_object_tag(allocate_jsobject((ctx), BOXED_EMBEDDED_PROPS, \
                                                 HTAG_BOXED_BOOLEAN))

#define number_object_object_p(o)  (remove_normal_number_object_tag(o))
#define string_object_object_p(o)  (remove_normal_string_object_tag(o))
#define boolean_object_object_p(o) (remove_normal_boolean_object_tag(o))

#define number_eprop(o,t,i)                                     \
  (*(t*)&(remove_normal_number_object_tag(o))->eprop[i])
#define number_object_value(o)                          \
  number_eprop(o, JSValue, BOXED_XPROP_INDEX_VALUE)

#define string_eprop(o,t,i)                                     \
  (*(t*)&(remove_normal_string_object_tag(o))->eprop[i])
#define string_object_value(o)                          \
  string_eprop(o, JSValue, BOXED_XPROP_INDEX_VALUE)

#define boolean_eprop(o,t,i)                                     \
  (*(t*)&(remove_normal_boolean_object_tag(o))->eprop[i])
#define boolean_object_value(o)                         \
  boolean_eprop(o, JSValue, BOXED_XPROP_INDEX_VALUE)


/*
 * Iterator
 * tag == T_GENERIC
 */
typedef struct iterator {
  uint64_t size;        /* array size */
  uint64_t index;       /* array index */
  JSValue *body;        /* pointer to a C array */
} Iterator;

#define make_iterator()                                 \
  (put_normal_iterator_tag(allocate_iterator()))
#define iterator_size(i)                        \
  ((remove_normal_iterator_tag(i))->size)
#define iterator_index(i)                       \
  ((remove_normal_iterator_tag(i))->index)
#define iterator_body(i)                        \
  ((remove_normal_iterator_tag(i))->body)
#define iterator_body_index(a,i)                \
  ((remove_normal_iterator_tag(a))->body[i])


  /*
   * Flonum
   */
#define flonum_value(p)      (normal_flonum_value(p))
#define double_to_flonum(n)  (double_to_normal_flonum(n))
#define int_to_flonum(i)     (int_to_normal_flonum(i))
#define cint_to_flonum(i)    (cint_to_normal_flonum(i))
#define flonum_to_double(p)  (normal_flonum_to_double(p))
#define flonum_to_cint(p)    (normal_flonum_to_cint(p))
#define flonum_to_int(p)     (normal_flonum_to_int(p))
#define is_nan(p)            (normal_flonum_is_nan(p))

/*
 * FlonumCell
 * tag == T_FLONUM
 */
typedef struct flonum_cell {
  double value;
} FlonumCell;

#define normal_flonum_value(p)      ((remove_normal_flonum_tag(p))->value)
#define double_to_normal_flonum(n)  (put_normal_flonum_tag(allocate_flonum(n)))
#define int_to_normal_flonum(i)     cint_to_flonum(i)
#define cint_to_normal_flonum(i)    double_to_flonum((double)(i))
#define normal_flonum_to_double(p)  flonum_value(p)
#define normal_flonum_to_cint(p)    ((cint)(flonum_value(p)))
#define normal_flonum_to_int(p)     ((int)(flonum_value(p)))
#define normal_flonum_is_nan(p)                         \
  (is_flonum((p))? isnan(flonum_to_double((p))): 0)

/*
 * String
 */
#define string_value(p)   (normal_string_value(p))
#define string_hash(p)    (normal_string_hash(p))
#define string_length(p)  (normal_string_length(p))
#define cstr_to_string(ctx,str) (cstr_to_normal_string((ctx),(str)))
#define ejs_string_concat(ctx,str1,str2)                \
  (ejs_normal_string_concat((ctx),(str1),(str2)))

  /*
   * StringCell
   * tag == T_STRING
   */
typedef struct string_cell {
#ifdef STROBJ_HAS_HASH
  uint32_t hash;           /* hash value before computing mod */
  uint32_t length;         /* length of the string */
#endif
  char value[BYTES_IN_JSVALUE];
} StringCell;

#define string_to_cstr(p) (string_value(p))

#define normal_string_value(p)   ((remove_normal_string_tag(p))->value)

#ifdef STROBJ_HAS_HASH
#define normal_string_hash(p)       ((remove_normal_string_tag(p))->hash)
#define normal_string_length(p)     ((remove_normal_string_tag(p))->length)
#else
#define normal_string_hash(p)       (calc_hash(string_value(p)))
#define normal_string_length(p)     (strlen(string_value(p)))
#endif /* STROBJ_HAS_HASH */

#define cstr_to_normal_string(ctx, str)  (cstr_to_string_ool((ctx), (str)))
/* TODO: give a nice name to ejs_string_concat
 *       (string_concat is used for builtin function) */
#define ejs_normal_string_concat(ctx, str1, str2)       \
  (string_concat_ool((ctx), (str1), (str2)))

/*
 * Object header
 *
 *  ---------------------------------------------------
 *  |  object size in bytes  |    header tag          |
 *  ---------------------------------------------------
 *  63                     32 31                     0
 */

/* change name: OBJECT_xxx -> HEADER_xxx (ugawa) */
#define HEADER_SIZE_OFFSET   (32)
#define HEADER_TYPE_MASK     ((uint64_t)0x000000ff)
#define HEADER_SHARED_MASK   ((uint64_t)0x80000000)
#define FUNCTION_ATOMIC_MASK ((uint64_t)0x40000000)

#define make_header(s, t) (((uint64_t)(s) << HEADER_SIZE_OFFSET) | (t))

/*
 * header tags for non-JS objects
 */
/* HTAG_FREE is defined in gc.c */
#define HTAG_PROP           (0x11) /* Array of JSValues (partly uninitialised) */
#define HTAG_ARRAY_DATA     (0x12) /* Array of JSValues */
#define HTAG_FUNCTION_FRAME (0x13) /* FunctionFrame */
#define HTAG_STR_CONS       (0x14) /* StrCons */
#define HTAG_CONTEXT        (0x15) /* Context */
#define HTAG_STACK          (0x16) /* Array of JSValues */
#ifdef HIDDEN_CLASS
#define HTAG_HIDDEN_CLASS   (0x17) /* HiddenClass */
#endif
#define HTAG_HASHTABLE      (0x18)
#define HTAG_HASH_BODY      (0x19)
#define HTAG_HASH_CELL      (0x1A)

/*
 * Fixnum
 * tag == T_FIXNUM
 *
 * In 64-bits environment, C's `int' is a 32-bits integer.
 * A fixnum value (61-bits signed integer) cannot be represented in an int. 
 * So we use `cint' to represent a fixnum value.
 */

typedef int64_t cint;
typedef uint64_t cuint;


/* #define fixnum_to_int(p) (((int64_t)(p)) >> TAGOFFSET) */
#define fixnum_to_cint(p) (((cint)(p)) >> TAGOFFSET)
#define fixnum_to_int(p)  ((int)fixnum_to_cint(p))
#define fixnum_to_double(p) ((double)(fixnum_to_cint(p)))

/*
 * #define int_to_fixnum(f) \
 * ((JSValue)(put_tag((((uint64_t)(f)) << TAGOFFSET), T_FIXNUM)))
 */
#define int_to_fixnum(f)    cint_to_fixnum(((cint)(f)))
#define cint_to_fixnum(f)   put_tag(((uint64_t)(f) << TAGOFFSET), T_FIXNUM)

/* #define double_to_fixnum(f) int_to_fixnum((int64_t)(f)) */
#define double_to_fixnum(f) cint_to_fixnum((cint)(f))

#define is_fixnum_range_cint(n)                                 \
  ((MIN_FIXNUM_CINT <= (n)) && ((n) <= MAX_FIXNUM_CINT))

#define is_integer_value_double(d) ((d) == (double)((cint)(d)))

#define is_fixnum_range_double(d)                                       \
  (is_integer_value_double(d) && is_fixnum_range_cint((cint)(d)))

#define in_fixnum_range(dval)                           \
  ((((double)(dval)) == ((double)((int64_t)(dval))))    \
   && ((((int64_t)(dval)) <= MAX_FIXNUM_INT)            \
       && (((int64_t)(dval)) >= MIN_FIXNUM_INT)))

#define in_flonum_range(ival)                           \
  ((ival ^ (ival << 1))                                 \
   & ((int64_t)1 << (BITS_IN_JSVALUE - TAGOFFSET)))

#define half_fixnum_range(ival)                                         \
  (((MIN_FIXNUM_CINT / 2) <= (ival)) && ((ival) <= (MAX_FIXNUM_CINT / 2)))

#define FIXNUM_ZERO (cint_to_fixnum((cint)0))
#define FIXNUM_ONE  (cint_to_fixnum((cint)1))
#define FIXNUM_TEN  (cint_to_fixnum((cint)10))

#define MAX_FIXNUM_CINT (((cint)(1) << (BITS_IN_JSVALUE - TAGOFFSET - 1)) - 1)
#define MIN_FIXNUM_CINT (-MAX_FIXNUM_CINT-1)


#define cint_to_number(n)                                               \
  (is_fixnum_range_cint((n))? cint_to_fixnum((n)): cint_to_flonum((n)))

#define number_to_double(p)                                     \
  ((is_fixnum(p)? fixnum_to_double(p): flonum_to_double(p)))
#define double_to_number(d)                                             \
  ((is_fixnum_range_double(d))? double_to_fixnum(d): double_to_flonum(d))

/*
 * Special
 * tag == T_SPECIAL
 */
#define SPECIALOFFSET           (TAGOFFSET + 1)
#define SPECIALMASK             ((uint64_t)(1 << SPECIALOFFSET) - 1)

#define make_special(spe,t)     ((JSValue)((spe) << SPECIALOFFSET | (t)))
#define special_tag(p)          ((uint64_t)(p) & SPECIALMASK)
#define special_equal_tag(p,t)  (special_tag((p)) == (t))

/*
 * Special - Boolean
 */
#define T_BOOLEAN         ((0x1 << TAGOFFSET) | T_SPECIAL)
#define JS_TRUE           make_special(1, T_BOOLEAN)
#define JS_FALSE          make_special(0, T_BOOLEAN)

#define is_boolean(p)     (special_tag((p)) == T_BOOLEAN)
#define is_true(p)        ((p) == JS_TRUE)
#define is_false(p)       ((p) == JS_FALSE)
#define int_to_boolean(e) ((e) ? JS_TRUE : JS_FALSE)

#define true_false(e)     ((e) ? JS_TRUE : JS_FALSE)
#define false_true(e)     ((e) ? JS_FALSE : JS_TRUE)

/*
 * Special - Others
 */
#define T_OTHER           ((0x0 << TAGOFFSET) | T_SPECIAL)
#define JS_NULL           make_special(0, T_OTHER)
#define JS_UNDEFINED      make_special(1, T_OTHER)

#define is_null_or_undefined(p)  (special_tag((p)) == T_OTHER)
#define is_null(p)               ((p) == JS_NULL)
#define is_undefined(p)          ((p) == JS_UNDEFINED)

/*
 * Primitive is either number, boolean, or string.
 */
#define is_primitive(p) (!is_object(p) && !is_null_or_undefined(p))

/*
 * Set a specified property to an object where property name is given
 * by a string object or a C string.
 */

#define set_prop_none(c, o, s, v)                       \
  set_prop_with_attribute(c, o, s, v, ATTR_NONE)
#define set_prop_all(c, o, s, v) set_prop_with_attribute(c, o, s, v, ATTR_ALL)
#define set_prop_de(c, o, s, v) set_prop_with_attribute(c, o, s, v, ATTR_DE)
#define set_prop_ddde(c, o, s, v)                       \
  set_prop_with_attribute(c, o, s, v, ATTR_DDDE)

#define set___proto___none(c, o, v)                     \
  set_prop_none(c, o, gconsts.g_string___proto__, v)
#define set___proto___all(c, o, v)                      \
  set_prop_all(c, o, gconsts.g_string___proto__, v)
#define set___proto___de(c, o, v)                       \
  set_prop_de(c, o, gconsts.g_string___proto__, v)
#define set_prototype_none(c, o, v)                     \
  set_prop_none(c, o, gconsts.g_string_prototype, v)
#define set_prototype_all(c, o, v)                      \
  set_prop_all(c, o, gconsts.g_string_prototype, v)
#define set_prototype_de(c, o, v)                       \
  set_prop_de(c, o, gconsts.g_string_prototype, v)

#define set_obj_cstr_prop(c, o, s, v, attr)                             \
  set_prop_with_attribute(c, o, cstr_to_string((c),(s)), v, attr)
#define set_obj_cstr_prop_none(c, o, s, v)      \
  set_obj_cstr_prop(c, o, s, v, ATTR_NONE)

#define get___proto__(o, r) get_prop(o, gconsts.g_string___proto__, r)
#endif

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
