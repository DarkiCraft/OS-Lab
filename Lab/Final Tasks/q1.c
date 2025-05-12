#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  size_t n;
  int* arr;
} Array;

void* Sort(void* arg) {
  Array* a = (Array*)arg;

  for (size_t i = 0; i < a->n; i++) {
    for (size_t j = i + 1; j < a->n; j++) {
      if (a->arr[i] > a->arr[j]) {
        int t     = a->arr[i];
        a->arr[i] = a->arr[j];
        a->arr[j] = t;
      }
    }
  }

  return NULL;
}

Array* Merge(Array* l, Array* r) {
  Array* m = malloc(sizeof(Array));
  m->n     = l->n + r->n;
  m->arr   = malloc(m->n * sizeof(int));

  size_t i = 0, j = 0, k = 0;
  while (i < l->n && j < r->n) {
    if (l->arr[i] <= r->arr[j]) {
      m->arr[k++] = l->arr[i++];
    } else {
      m->arr[k++] = r->arr[j++];
    }
  }
  while (i < l->n) m->arr[k++] = l->arr[i++];
  while (j < r->n) m->arr[k++] = r->arr[j++];
  return m;
}

int main() {
  Array a;
  printf("Enter number of elements: ");
  if (scanf("%zu", &a.n) != 1) {
    perror("scanf");
    exit(EXIT_FAILURE);
  }

  a.arr = malloc(a.n * sizeof(int));
  if (a.arr == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  printf("Enter %zu elements: ", a.n);
  for (size_t i = 0; i < a.n; i++) {
    if (scanf("%d", &a.arr[i]) != 1) {
      perror("scanf");
      free(a.arr);
      exit(EXIT_FAILURE);
    }
  }

  Array left  = {a.n / 2, a.arr};
  Array right = {a.n - left.n, a.arr + left.n};

  pthread_t t1, t2;
  if (pthread_create(&t1, NULL, Sort, &left) != 0 || pthread_create(&t2, NULL, Sort, &right) != 0) {
    perror("pthread_create");
  }

  if (pthread_join(t1, NULL) != 0 || pthread_join(t2, NULL) != 0) {
    perror("pthread_join");
  }

  Array* result = Merge(&left, &right);

  printf("Sorted array: ");
  for (size_t i = 0; i < result->n; i++) {
    printf("%d ", result->arr[i]);
  }
  printf("\n");

  free(a.arr);
  free(result->arr);
  free(result);
}