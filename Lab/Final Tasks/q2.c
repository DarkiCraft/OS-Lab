#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MATRIX_SIZE 3

typedef struct {
  int data[MATRIX_SIZE][MATRIX_SIZE];
} Matrix;

Matrix a;
Matrix b;
Matrix r;

typedef struct {
  size_t a_idx;
  size_t b_idx;
} InnerProductParamter;

void* InnerProduct(void* arg) {
  if (arg == NULL) {
    fprintf(stderr, "NULL arg passed\n");
    return NULL;
  }

  InnerProductParamter* param = (InnerProductParamter*)arg;

  int sum = 0;
  for (size_t i = 0; i < MATRIX_SIZE; i++) {
    sum += a.data[param->a_idx][i] * b.data[i][param->b_idx];
  }
  r.data[param->a_idx][param->b_idx] = sum;

  free(arg);

  return NULL;
}

int main() {
  printf("Enter elements for matrix A [3x3]:\n");
  for (size_t i = 0; i < MATRIX_SIZE; i++) {
    for (size_t j = 0; j < MATRIX_SIZE; j++) {
      if (scanf("%d", &a.data[i][j]) != 1) {
        perror("scanf");
        exit(EXIT_FAILURE);
      }
    }
  }

  printf("Enter elements for matrix B [3x3]:\n");
  for (size_t i = 0; i < MATRIX_SIZE; i++) {
    for (size_t j = 0; j < MATRIX_SIZE; j++) {
      if (scanf("%d", &b.data[i][j]) != 1) {
        perror("scanf");
        exit(EXIT_FAILURE);
      }
    }
  }

  pthread_t ids[MATRIX_SIZE][MATRIX_SIZE];

  for (size_t i = 0; i < MATRIX_SIZE; i++) {
    for (size_t j = 0; j < MATRIX_SIZE; j++) {
      InnerProductParamter* arg = (InnerProductParamter*)malloc(sizeof(InnerProductParamter));
      if (arg == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
      }

      arg->a_idx = i;
      arg->b_idx = j;

      if (pthread_create(&ids[i][j], NULL, InnerProduct, (void*)arg) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
      }
    }
  }

  for (size_t i = 0; i < MATRIX_SIZE; i++) {
    for (size_t j = 0; j < MATRIX_SIZE; j++) {
      if (pthread_join(ids[i][j], NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
      }
    }
  }

  printf("Result:\n");
  for (size_t i = 0; i < MATRIX_SIZE; i++) {
    for (size_t j = 0; j < MATRIX_SIZE; j++) {
      printf("%d ", r.data[i][j]);
    }
    putchar('\n');
  }
}