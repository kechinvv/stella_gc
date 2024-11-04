#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "runtime.h"
#include "gc.h"

#define MEM_FOR_GARBAGE 200

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
 return p != NULL && p >= from_space && p < (from_space + MEM_FOR_GARBAGE);
}

int points_to_to_space(void *p){
 return p != NULL && p >= to_space && p < (to_space + MEM_FOR_GARBAGE);
}

void check_enomem(size_t size_in_bytes) {
  if (last_added + size_in_bytes > limit) { 
      print_gc_state(); 
      print_gc_alloc_stats();
      printf("EXIT ENOMEM\n");
      exit(ENOMEM);
  }
}

void chase(stella_object* p){
  do
  {
    stella_object* q = next;
    int fields_count = STELLA_OBJECT_HEADER_FIELD_COUNT(p->object_header);
    size_t size_in_bytes = sizeof(void *) * (fields_count + 1);
    check_enomem(size_in_bytes);

    next += size_in_bytes; //todo + size of object? only pointers with same size or primitives?
    last_added = next;  //while gc working - new obj to the end, forwarded to the next

    stella_object* r = NULL;
    
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

    return p;
}

void gc_iter(size_t size_in_bytes){
  long forwarded = 0l;
  long before_size = next;
  while (scan < next) {
    if (forwarded >= before_size) return;
    
    stella_object *obj = (stella_object*) scan;
    int fields_count = STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
    for (int i = 0; i < fields_count; i++) {
      obj->object_fields[i] = forward(obj->object_fields[i]);
    }
    forwarded = next - before_size; 
    scan += sizeof(void *) * (fields_count+1);
    printf("Forwarded %ld\n", forwarded);
  }
  
  gc_collecting = 0;
  next = to_space;
  scan = to_space;
}

void prepare() {
    void* buf = from_space;
    from_space = to_space;
    to_space = buf;
}

void gc_run(){
  gc_collecting = 1;
  next = scan = to_space;
  limit = to_space + MEM_FOR_GARBAGE;
  last_added = to_space;


  for (int root_i = 0; root_i < gc_roots_top; root_i++) {
      void **root = gc_roots[root_i];
      *root = forward(*root);
  }

  prepare();
}


void* gc_alloc(size_t size_in_bytes) {
   printf("\n\nGC_ALLOC for %d\n", size_in_bytes);

  if (next == NULL) {
      from_space = malloc(MEM_FOR_GARBAGE);
      to_space = malloc(MEM_FOR_GARBAGE);
      
      last_added = from_space;
      next = to_space;
      limit = from_space + MEM_FOR_GARBAGE;
  }

  print_gc_state();

  total_allocated_bytes += size_in_bytes;
  total_allocated_objects += 1;
  max_allocated_bytes = total_allocated_bytes;
  max_allocated_objects = total_allocated_objects;

  printf("BEFORE GC RUN\n");


  if (gc_collecting == 0 && last_added + size_in_bytes > limit) {
      printf("GC RUN\n");
      gc_run();
  }

  check_enomem(size_in_bytes);

  void* ptr_to_write;
  if (gc_collecting == 0) {
    ptr_to_write = last_added;
    last_added += size_in_bytes;
  } else {
    printf("GC_ITER\n");
    gc_iter(size_in_bytes);
    check_enomem(size_in_bytes);

    limit -= size_in_bytes;
    ptr_to_write = limit;

    printf("\nafter iter\n");
    print_gc_state();
    printf("\n\n");
  }

  printf("\nafter allocate\n");
  print_gc_state();

  
  printf("\nafter iter\n");
  print_gc_state();
  printf("\n\n");


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
  printf("GC_STATE\n");

  printf("To space start: ...%ld \n", (long)to_space % 100000);
  printf("Next: ...%ld \n",(long) next % 100000);
  printf("From space start: ...%ld \n", (long) from_space % 100000);
  printf("Last added: ...%ld \n", (long) last_added % 100000);
  printf("Limit: ...%ld \n", (long) limit % 100000);
  printf("Free space: %'ld / %'d \n", (limit - last_added + 1), MEM_FOR_GARBAGE);
  printf("Collecting now?: %'d \n", gc_collecting);
  printf("Scan:  %'ld \n", (long) scan % 100000);
  print_gc_roots();
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
  printf("GC_READ_BARRIER\n");

  stella_object* s_obj = (stella_object *) object;
  if (gc_collecting == 1 && points_to_to_space(s_obj->object_fields[field_index])) {  //old from space
      s_obj->object_fields[field_index] = forward(s_obj->object_fields[field_index]);
  }
  total_reads += 1;
}

