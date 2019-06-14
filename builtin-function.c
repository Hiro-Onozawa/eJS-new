/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

#include "prefix.h"
#define EXTERN extern
#include "header.h"

#define not_implemented(s)                                              \
  LOG_EXIT("%s is not implemented yet\n", (s)); set_a(context, JS_UNDEFINED)

#define MAX_FUNCTION_APPLY_ARGUMENTS 64

BUILTIN_FUNCTION(function_constr) {
  not_implemented("function_constr");
}

BUILTIN_FUNCTION(function_apply) {
  builtin_prologue();
  JSValue fn = args[0];
  JSValue thisobj = args[1];
  JSValue as = args[2];
  JSValue ret = JS_UNDEFINED;
  JSValue arguments[MAX_FUNCTION_APPLY_ARGUMENTS];
  int alen = 0;

  if (na >= 2 && is_array(as)) {
    int i;

    alen = array_length(as);
    if (alen > MAX_FUNCTION_APPLY_ARGUMENTS)
      LOG_EXIT("apply: too many arguments (%d) in the second argument",
               alen);
    for (i = 0; i < alen; i++)
      arguments[i] = array_body_index(as, i);
  } else if (na < 2)
    LOG_EXIT("apply: too few arguments");
  else
    LOG_EXIT("apply: the second argument is expected to be an array");
  if (is_function(fn)) {
    ret = invoke_function(context, thisobj, fn, TRUE, arguments, alen);
  } else if (is_builtin(fn)) {
    /* call_builtin(context, fn, na, true, false); */
    not_implemented("function_apply");
  } else {
    LOG_EXIT("apply: the receiver has to be a function/builtin");
  }
  set_a(context, ret);
}

ObjBuiltinProp function_funcs[] = {
  { "apply", function_apply, 2, ATTR_DE },
  { NULL, NULL, 0, ATTR_DE }
};

void init_builtin_function(Context *ctx)
{
  JSValue proto;

  gconsts.g_function =
    new_normal_builtin_with_constr(ctx, function_constr, function_constr, 0);
  gconsts.g_function_proto = proto = new_big_predef_object(ctx);
  set_prototype_all(ctx, gconsts.g_function, proto);
  {
    ObjBuiltinProp *p = function_funcs;
    while (p->name != NULL) {
      set_obj_cstr_prop(ctx, proto, p->name,
                        new_normal_builtin(ctx, p->fn, p->na), p->attr);
      p++;
    }
  }
}
