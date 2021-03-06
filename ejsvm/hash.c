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

#define HASH_SKIP (27)

/* #define REHASH_THRESHOLD (0.5) */
#define REHASH_THRESHOLD (0.9)


#ifdef PROPERTY_MAP_HASHTABLE
static int rehash(HashTable *table);
static int next_hash_cell(HashTable *table,
                          HashIterator *iter, HashCell **pp,
                          Attribute is_transition);


static HashCell** alloc_hash_body(Context *ctx, int size)
{
  HashCell **body = (HashCell **) gc_malloc(ctx, sizeof(HashCell*) * size,
                                            CELLT_HASH_BODY);
  int i;
  for (i = 0; i < size; i++)
    body[i] = NULL;
  return body;
}

static HashCell* alloc_hash_cell(Context *ctx)
{
  HashCell *cell = (HashCell *) gc_malloc(ctx,
                                          sizeof(HashCell), CELLT_HASH_CELL);
  cell->next = NULL;
  return cell;
}
#endif /* PROPERTY_MAP_HASHTABLE */

/*
 * initializes a hash table with the specified size
 */
#ifdef PROPERTY_MAP_HASHTABLE
HashTable *hash_create(Context *ctx, unsigned int size) {
  HashCell **body;
  HashTable *table;
  int i;

  if (size == 0)
    size = 1;

  table = (HashTable *)gc_malloc(ctx,
                                 sizeof(HashTable), CELLT_HASHTABLE);
  table->body = NULL;  /* tell GC no to follow this pointer */
  GC_PUSH(table);

  body = (HashCell **) gc_malloc(ctx, sizeof(HashCell*) * size,
                                 CELLT_HASH_BODY);
  for (i = 0; i < size; i++)
    body[i] = NULL;
  table->body = body;
  table->size = size;
  table->filled = 0;
  table->entry_count = 0;
  GC_POP(table);
  return table;
}
#else /* PROPERTY_MAP_HASHTABLE */
HashTable *hash_create(Context *ctx, unsigned int size)
{
  HashTable *table;
  int i;
  table = (HashTable *)
    gc_malloc(ctx, sizeof(HashTable) + sizeof(struct property) * size,
              CELLT_HASHTABLE);
  table->n_props = size; /* TODO: eliminate duplication */
  table->transitions = NULL;
  for (i = 0; i < size; i++) {
    table->entry[i].key = JS_UNDEFINED;
    table->entry[i].attr = 0;
  }

  return table;
}
#endif /* PROPERTY_MAP_HASHTABLE */

/*
 * obtains the value and attribute associated with a given key
 */
#ifdef PROPERTY_MAP_HASHTABLE
int hash_get_with_attribute(HashTable *table, HashKey key, HashData *data,
                            Attribute *attr) {
  uint32_t hval;
  HashCell *cell;

  hval = string_hash(key) % table->size;
  for (cell = table->body[hval]; cell != NULL; cell = cell->next)
    if ((JSValue)(cell->entry.key) == key) {
      /* found */
      if (data != NULL) *data = cell->entry.data;
      if (attr != NULL) *attr = cell->entry.attr;
      /* printf("hash_get_with_attr: success, *data = %d\n", *data); */
      return HASH_GET_SUCCESS;
    }
  /* not found */
  /* printf("hash_get_with_attr: fail\n"); */
  return HASH_GET_FAILED;
}
#else /* PROPERTY_MAP_HASHTABLE */
int hash_get_with_attribute(HashTable *table, HashKey key, HashData *data,
                            Attribute *attr)
{
  int i;

  assert(is_string(key));
  
  for (i = 0; i < table->n_props; i++)
    if (table->entry[i].key == key) {
      assert(!is_transition(table->entry[i].attr));
      if (data != NULL) data->u.index = i;
      if (attr != NULL) *attr = table->entry[i].attr;
      return HASH_GET_SUCCESS;
    }
  if (table->transitions != NULL) {
    TransitionTable *ttable = table->transitions;
    for (i = 0; i < ttable->n_transitions; i++)
      if (ttable->transition[i].key == key) {
        if (data != NULL) data->u.pm = ttable->transition[i].pm;
        if (attr != NULL) *attr = ATTR_TRANSITION;
        return HASH_GET_SUCCESS;
      }
  }

  return HASH_GET_FAILED;
}
#endif /* PROPERTY_MAP_HASHTABLE */

/*
 * registers a value to a hash table under a given key with an attribute
 */
#ifdef PROPERTY_MAP_HASHTABLE
static int hash_put_with_attribute(Context *ctx, HashTable* table,
                                   HashKey key, HashData data, Attribute attr)
{
  HashCell* cell;
  uint32_t index;

  index = string_hash(key) % table->size;
  for (cell = table->body[index]; cell != NULL; cell = cell->next) {
    if (cell->entry.key == key) {
      /* found */
      if (!is_readonly(cell->entry.attr)) {
        cell->deleted = false;
        cell->entry.data = data;
        cell->entry.attr = attr;
        return HASH_PUT_SUCCESS;
      } else
        return HASH_PUT_FAILED;
    }
  }
  /* not found */
  GC_PUSH(table);
  GC_PUSH(key);
  if (is_transition(attr))
    GC_PUSH(data);
  cell = alloc_hash_cell(ctx);
  cell->next = table->body[index];
  table->body[index] = cell;
  cell->deleted = false;
  cell->entry.key = key;
  cell->entry.data = data;
  cell->entry.attr = attr;
  if (cell->next == NULL) {
    table->entry_count++;
    if (table->entry_count > REHASH_THRESHOLD * table->size)
      rehash(table);
  }
  if (is_transition(attr))
    GC_POP(data);
  GC_POP(key);
  GC_POP(table);
  return HASH_PUT_SUCCESS;
}
int hash_put_property(Context *ctx, HashTable *table,
                      HashKey key, uint32_t index, Attribute attr)
{
  HashData data;
  data.u.index = index;
  return hash_put_with_attribute(ctx, table, key, data, attr);
}

void hash_put_transition(Context *ctx, HashTable *table,
                         HashKey key, PropertyMap *pm)
{
  HashData data;
  data.u.pm = pm;
  hash_put_with_attribute(ctx, table, key, data, ATTR_TRANSITION);
}
#else /* PROPERTY_MAP_HASHTABLE */
int hash_put_property(Context *ctx, HashTable *table,
                      HashKey key, uint32_t index, Attribute attr)
{
  assert(table->entry[index].key == JS_UNDEFINED ||
         table->entry[index].key == key);
  assert(!is_transition(attr));
  
  if (is_readonly(table->entry[index].attr))
    return HASH_PUT_FAILED;
  table->entry[index].key = key;
  table->entry[index].attr = attr;
  return HASH_PUT_SUCCESS;
}

void hash_put_transition(Context *ctx, HashTable *table,
                         HashKey key, PropertyMap *pm)
{
  int i;
  TransitionTable *ttable;
  int n_transitions;
  if (table->transitions == NULL)
    n_transitions = 1;
  else
    n_transitions = table->transitions->n_transitions + 1;
  
  GC_PUSH3(table, key, pm);
  ttable = (TransitionTable *)
    gc_malloc(ctx, sizeof(TransitionTable) +
              sizeof(struct transition) * n_transitions,
              CELLT_TRANSITIONS);
  GC_POP3(pm, key, table);

  for (i = 0; i < n_transitions - 1; i++)
    ttable->transition[i] = table->transitions->transition[i];
  ttable->transition[i].key = key;
  ttable->transition[i].pm = pm;
  ttable->n_transitions = n_transitions;
  table->transitions = ttable;
}
#endif /* PROPERTY_MAP_HASHTABLE */


/*
 * deletes the hash data
 */
#ifdef PROPERTY_MAP_HASHTABLE
int hash_delete(HashTable *table, HashKey key) {
  HashCell *cell, *prev;
  uint32_t index;

  index = string_hash(key) % table->size;
  for (prev = NULL, cell = table->body[index]; cell != NULL;
       prev = cell, cell = cell->next) {
    if (cell->entry.key == key) {
      /* found */
      if (!is_dont_delete(cell->entry.attr))
        return HASH_GET_FAILED;
      if (prev == NULL) {
        table->body[index] = cell->next;
      } else {
        prev->next = cell->next;
      }
      return HASH_GET_SUCCESS;
    }
  }
  return HASH_GET_FAILED;
}
#else /* PROPERTY_MAP_HASHTABLE */
int hash_delete(HashTable *table, HashKey key) {
  /* TO BE IMPLEMENTED */
  abort();
  return HASH_GET_FAILED;
}
#endif /* PROPERTY_MAP_HASHTABLE */

/*
 * copies a hash table
 * This function is used only for copying a hash table in a hidden class.
 * This function returns the number of copied properties.
 */
#ifdef PROPERTY_MAP_HASHTABLE
int hash_copy(Context *ctx, HashTable *from, HashTable *to) {
  int i, fromsize, tosize;
  HashCell *cell, *new;
  uint32_t index;
  int n, ec;

  fromsize = from->size;
  tosize = to->size;
  n = 0;
  ec = 0;
  cell = NULL;
  GC_PUSH3(from, to, cell);
  for (i = 0; i < fromsize; i++) {
    for (cell = from->body[i]; cell != NULL; cell = cell->next) {
      /* we do not copy the transition entry. */
      if (is_transition(cell->entry.attr))
        continue;
      if (cell->deleted)
        continue;
      index = string_hash(cell->entry.key) % tosize;
      new = alloc_hash_cell(ctx);
      new->deleted = false;
      new->entry = cell->entry;
      if (to->body[index] == NULL) ec++;   /* increments entry count */
      new->next = to->body[index];
      to->body[index] = new;
      n++;
    }
  }
  to->entry_count = ec;
  to->filled = from->filled;
  GC_POP3(cell, to, from);
  return n;
}
#else /* PROPERTY_MAP_HASHTABLE */
int hash_copy(Context *ctx, HashTable *from, HashTable *to)
{
  int i, n = 0;
  assert(from->n_props <= to->n_props);
  
  for (i = 0; i < from->n_props; i++) {
    if (from->entry[i].key != JS_UNDEFINED) {
      n++;
      to->entry[i].key = from->entry[i].key;
    }
  }

  return n;
}
#endif /* PROPERTY_MAP_HASHTABLE */

#ifdef PROPERTY_MAP_HASHTABLE
static int rehash(HashTable *table) {
  int size = table->size;
  int newsize = size * 2;
  HashIterator iter;
  HashCell *p;
  HashCell** newhash;

  GC_PUSH(table);
  newhash = alloc_hash_body(NULL, newsize);
  GC_PUSH(newhash);

  iter = createHashPropertyIterator(table).i;
  while (next_hash_cell(table, &iter, &p, ATTR_NONE) != FAIL) {
    uint32_t index = string_hash(p->entry.key) % newsize;
    p->next = newhash[index];
    newhash[index] = p;
  }
  iter = createHashTransitionIterator(table).i;
  while (next_hash_cell(table, &iter, &p, ATTR_TRANSITION) != FAIL) {
    uint32_t index = string_hash(p->entry.key) % newsize;
    p->next = newhash[index];
    newhash[index] = p;
  }
  table->body = newhash;
  table->size = newsize;

  GC_POP2(newhash, table);
  return 0;
}
#endif /* PROPERTY_MAP_HASHTABLE */

#ifdef PROPERTY_MAP_HASHTABLE
/*
 * Find the next cell.
 * param iter: Startig point of serach.  `iter->p == NULL' directs to
 * start searching from the begining of the list at `iter->index + 1'.
 */
static void advance_iterator(HashTable *table, HashIterator *iter,
                             Attribute is_trans)
{
  HashCell *p;
  int i;
  
  for (p = iter->p; p != NULL; p = p->next)
    if (!p->deleted &&
        ((is_trans ^ p->entry.attr) & ATTR_TRANSITION) == 0) {
      iter->p = p;
      return;
    }
  for (i = iter->index + 1; i < table->size; i++)
    for (p = table->body[i]; p != NULL; p = p->next)
      if (!p->deleted &&
          ((is_trans ^ p->entry.attr) & ATTR_TRANSITION) == 0) {
        iter->p = p;
        iter->index = i;
        return;
      }
  iter->p = NULL;
}


static int next_hash_cell(HashTable *table,
                        HashIterator *iter, HashCell **pp,
                        Attribute is_transition)
{
  /* Care the case where the current cell is deleted
   * after the last call of `next_hash_cell'. */
  advance_iterator(table, iter, is_transition);

  if (iter->p == NULL)
    return FAIL;
  *pp = iter->p;
  iter->p = iter->p->next;
  advance_iterator(table, iter, is_transition);

  return SUCCESS;
}

HashPropertyIterator createHashPropertyIterator(HashTable *table)
{
  HashPropertyIterator iter;
  iter.i.p = NULL;
  iter.i.index = -1;
  advance_iterator(table, &iter.i, ATTR_NONE);
  return iter;
}

HashTransitionIterator createHashTransitionIterator(HashTable *table)
{
  HashTransitionIterator iter;
  iter.i.p = NULL;
  iter.i.index = -1;
  advance_iterator(table, &iter.i, ATTR_TRANSITION);
  return iter;
}

int nextHashPropertyCell(HashTable *table,
                         HashPropertyIterator *iter,
                         JSValue *key,
                         uint32_t *index,
                         Attribute *attr)
{
  HashCell *cell;
  if (next_hash_cell(table, &iter->i, &cell, ATTR_NONE) == FAIL)
    return FAIL;
  *key = cell->entry.key;
  *index = cell->entry.data.u.index;
  *attr = cell->entry.attr;
  return SUCCESS;
}

int nextHashTransitionCell(HashTable *table,
                           HashTransitionIterator *iter,
                           HashTransitionCell **pp)
{
  return next_hash_cell(table, &iter->i, (HashCell**) pp, ATTR_TRANSITION);
}
#else /* PROPERTY_MAP_HASHTABLE */

HashPropertyIterator createHashPropertyIterator(HashTable *table)
{
  HashPropertyIterator iter;
  iter.i = 0;
  return iter;
}

int nextHashPropertyCell(HashTable *table,
                         HashPropertyIterator *iter,
                         JSValue *key,
                         uint32_t *index,
                         Attribute *attr)
{
  if (iter->i < table->n_props) {
    if (table->entry[iter->i].key != JS_UNDEFINED) {
      *key = table->entry[iter->i].key;
      *index = iter->i;
      *attr = table->entry[iter->i].attr;
      iter->i++;
      return SUCCESS;
    }
    iter->i++;
  }
  return FAIL;
}

HashTransitionIterator createHashTransitionIterator(HashTable *table)
{
  HashTransitionIterator iter;
  iter.i = 0;
  return iter;
}

int nextHashTransitionCell(HashTable *table,
                           HashTransitionIterator *iter,
                           HashTransitionCell **pp)
{
  if (table->transitions == NULL)
    return FAIL;
  while (iter->i < table->transitions->n_transitions) {
    if (table->transitions->transition[iter->i].key != JS_UNDEFINED) {
      *pp = &table->transitions->transition[iter->i];
      iter->i++;
      return SUCCESS;
    }
    iter->i++;
  }
  return FAIL;
}

#endif /* PROPERTY_MAP_HASHTABLE */

/*
 * prints a hash table (for debugging)
 */
#ifdef PROPERTY_MAP_HASHTABLE
void print_hash_table(HashTable *tab) {

  HashCell *p;
  unsigned int i, ec;

  printf("HashTable %p: size = %d, entry_count = %d\n",
         tab, tab->size, tab->entry_count);
  ec = 0;
  for (i = 0; i < tab->size; i++) {
    if ((p = tab->body[i]) == NULL) continue;
    ec++;
    do {
      printf(" (%d: (", i);
      printf("0x%"PRIJSValue" = ", p->entry.key); simple_print(p->entry.key);
      printf(", ");
      printf("0x%"PRIx64, p->entry.data.u.index);
      printf("))\n");
    } while ((p = p->next) != NULL);
    /* if (ec >= tab->entry_count) break; */
  }
  printf("end HashTable\n");

}
#else /* PROPERTY_MAP_HASHTABLE */
void print_hash_table(HashTable *tab) {
}
#endif /* PROPERTY_MAP_HASHTABLE */

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
