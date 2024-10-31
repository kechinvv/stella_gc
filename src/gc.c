#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "runtime.h"
#include "gc.h"

#define MEM_FOR_GARBAGE 256

/** Total allocated number of bytes (over the entire duration of the program). */
int total_allocated_bytes = 0;

/** Total allocated number of objects (over the entire duration of the program). */
int total_allocated_objects = 0;

int max_allocated_bytes = 0;
int max_allocated_objects = 0;

int total_reads = 0;
int total_writes = 0;

#define MAX_GC_ROOTS 1024

int gc_roots_max_size = 0;
int gc_roots_top = 0;
void **gc_roots[MAX_GC_ROOTS];

void *from_space;
void *to_space;
void *next;
void *last_added;
void *limit;
void* scan;

int gc_collecting = 0;


int points_to_from_space(void *p){
 return p >= from_space && p < (from_space + MEM_FOR_GARBAGE);
}

int points_to_to_space(void *p){
 return p >= to_space && p < (to_space + MEM_FOR_GARBAGE);
}


void chase(stella_object* p){
  do
  {
    stella_object* q = next;
    int fields_count = STELLA_OBJECT_HEADER_FIELD_COUNT(p->object_header)+1;
    for (int i = 0; i < fields_count; i++) next += sizeof(p->object_fields[i]);
    

    void* r = NULL;
    q->object_header = p->object_header;

    for (int i = 0; i < fields_count; i++) {
        q->object_fields[i] = p->object_fields[i];

        if (points_to_from_space(q->object_fields[i])
                && !points_to_to_space(((stella_object *) q->object_fields[i])->object_fields[0])) {
            r = q->object_fields[i];
        }
    }

    p->object_fields[0] = q;
    p = r;
  } while (p != NULL);
}

void* forward(void* p) {
    if (points_to_from_space(p)) {
        stella_object* stella_p =(stella_object *) p;
        void* p_f1 = stella_p->object_fields[0];
        if (points_to_to_space(p_f1)) {
            return p_f1;
        } else {
            chase(stella_p);
            return (void*) stella_p->object_fields[0];
        }
    }

    return (void*) p;
}

void gc_iter(size_t size_in_bytes){
  size_t forwarded = 0;
  size_t before_size = last_added - from_space;
  while (forwarded < size_in_bytes) {
    stella_object *obj = scan;
    obj = forward(obj);
    forwarded = before_size - ((size_t) (last_added - from_space)); 
    scan += forwarded;
    if (scan >= to_space + MEM_FOR_GARBAGE || scan == NULL) {
      gc_collecting = 0;
    }
  }

}

void prepare() {
    void* buf = from_space;
    from_space = to_space;
    to_space = buf;
    last_added = next;
    next = to_space;
    limit = from_space + MEM_FOR_GARBAGE-1;
}

void gc_run(){
  gc_collecting = 1;
  scan = next;

  for (int root_i = 0; root_i < gc_roots_top; root_i++) {
      void **root = gc_roots[root_i];
      *root = forward(*root);
  }

  prepare();
}


void* gc_alloc(size_t size_in_bytes) {
  if (next == NULL) {
      from_space = malloc(MEM_FOR_GARBAGE);
      to_space = malloc(MEM_FOR_GARBAGE);
      
      last_added = from_space;
      next = from_space;
      limit = to_space + MEM_FOR_GARBAGE-1;
  }

  total_allocated_bytes += size_in_bytes;
  total_allocated_objects += 1;
  max_allocated_bytes = total_allocated_bytes;
  max_allocated_objects = total_allocated_objects;
  

  if (gc_collecting == 0 && last_added + size_in_bytes > limit) {
      gc_run();
  }

  if (last_added + size_in_bytes > limit) {  
      exit(ENOMEM);
  }

  void* ptr_to_write;
  if (gc_collecting == 0) {
    ptr_to_write = last_added;
    last_added += size_in_bytes;
  } else {
    ptr_to_write = limit;
    limit -= size_in_bytes;
  }
  gc_iter(size_in_bytes);

  return ptr_to_write;
}

void print_gc_roots() {
  printf("ROOTS: ");
  for (int i = 0; i < gc_roots_top; i++) {
    printf("%p ", gc_roots[i]);
  }
  printf("\n");
}

void print_gc_alloc_stats() {
  printf("Total memory allocation: %'d bytes (%'d objects)\n", total_allocated_bytes, total_allocated_objects);
  printf("Maximum residency:       %'d bytes (%'d objects)\n", max_allocated_bytes, max_allocated_objects);
  printf("Total memory use:        %'d reads and %'d writes\n", total_reads, total_writes);
  printf("Max GC roots stack size: %'d roots\n", gc_roots_max_size);
}

void print_gc_state() {
  printf("To space start: %p \n", to_space);
  printf("Next: %p \n", next);
  printf("From space start: %p \n", from_space);
  printf("Last added: %p \n", last_added);
  printf("Limit: %p \n", limit);
  printf("Free space: %'lu / %'d", (last_added - limit + 1), MEM_FOR_GARBAGE);
  printf("Collecting now?: %'d \n", gc_collecting);
}



void gc_write_barrier(void *object, int field_index, void *contents) {
  total_writes += 1;
}

void gc_push_root(void **ptr){
  gc_roots[gc_roots_top++] = ptr;
  if (gc_roots_top > gc_roots_max_size) { gc_roots_max_size = gc_roots_top; }
}

void gc_pop_root(void **ptr){
  gc_roots_top--;
}

void gc_read_barrier(void *object, int field_index) {
  void* s_obj_f = ((stella_object *) object)->object_fields[field_index];
  if (gc_collecting && points_to_to_space(s_obj_f)) {  //old from space
      s_obj_f = forward(s_obj_f);
  }
  total_reads += 1;
}

