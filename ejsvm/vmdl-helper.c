/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

#define EXTERN
#include "header.h"

JSValue initialize_new_object(Context* context, JSValue con, JSValue o) {
  JSValue p;
  get_prop(con, gconsts.g_string_prototype, &p);
  if (!is_object(p)) p = gconsts.g_object_proto;
  set___proto___all(context, o, p);
  return o;
}

JSValue get_global_helper(Context* context, JSValue str) {
  JSValue ret;

  if (get_prop(context->global, str, &ret) == FAIL) {
    LOG_EXIT("GETGLOBAL: %s not found\n", string_to_cstr(str));
  }
  return ret;
}

JSValue instanceof_helper(JSValue v1, JSValue v2) {
  JSValue p;
  if (get_prop(v2, gconsts.g_string_prototype, &p) == SUCCESS) {
    while (get___proto__(v1, &v1) == SUCCESS) {
      if (v1 == p) {
	return JS_TRUE;
      }
    }
  }
  return JS_FALSE;
}

JSValue getarguments_helper(Context* context, int link, Subscript index) {
  FunctionFrame* fr = get_lp(context);
  int i;
  for (i = 0; i < link; i++) {
    fr = fframe_prev(fr);
  }
  JSValue arguments = fframe_arguments(fr);
  return get_array_prop(context, arguments, int_to_fixnum(index));
}

JSValue getlocal_helper(Context* context, int link, Subscript index) {
  FunctionFrame* fr = get_lp(context);
  int i;
  for (i = 0; i < link; i++) {
    fr = fframe_prev(fr);
  }
  return fframe_locals_idx(fr, index);
}

Displacement localret_helper(Context* context, int pc) {
  Displacement disp;
  int newpc;
  JSValue v;
  if (context->lcall_stack < 1) {
    newpc = -1;
  } else {
    context->lcall_stack_ptr--;
    v = get_array_prop(context, context->lcall_stack,
		       cint_to_number((cint) context->lcall_stack_ptr));
    newpc = number_to_cint(v);
  }
  disp = (Displacement) (newpc - pc);
  return disp;
}

void setarg_helper(Context* context, int link, Subscript index, JSValue v2) {
  FunctionFrame *fr;
  JSValue arguments;
  int i;

  fr = get_lp(context);
  for (i = 0; i < link; i++) fr = fframe_prev(fr);
  // assert(index < array_size(fframe_arguments(fr)));
  // array_body_index(fframe_arguments(fr), index) = v2;
  // TODO: optimize
  arguments = fframe_arguments(fr);
  set_array_prop(context, arguments, int_to_fixnum(index), v2);
}

void setfl_helper(Context* context, JSValue *regbase, int fp, int newfl) {
  int oldfl;

  oldfl = get_sp(context) - fp + 1;
  // printf("fp = %d, newfl = %d, fp + newfl = %d\n", fp, newfl, fp + newfl);
  if (fp + newfl > STACK_LIMIT)
    LOG_EXIT("register stack overflow\n");
  set_sp(context, fp + newfl - 1);
  while (++oldfl <= newfl)
    regbase[oldfl] = JS_UNDEFINED;
}

void setglobal_helper(Context* context, JSValue str, JSValue src) {
  if (set_prop_none(context, context->global, str, src) == FAIL)
    LOG_EXIT("SETGLOBAL: setting a value of %s failed\n", string_to_cstr(str));
}

void setlocal_helper(Context* context, int link, Subscript index, JSValue v2) {
  FunctionFrame *fr;
  int i;

  fr = get_lp(context);
  for (i = 0; i < link; i++) fr = fframe_prev(fr);
  fframe_locals_idx(fr, index) = v2;
}

JSValue nextpropnameidx_helper(JSValue itr) {
  JSValue res = JS_UNDEFINED;
  iterator_get_next_propname(itr, &res);
  return res;
}

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
