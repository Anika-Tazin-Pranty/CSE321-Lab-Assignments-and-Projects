#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int compare(const void *a, const void *b) { 
    return (*(int *)b - *(int *)a); 
}

int main() {
  int arr[] = {10, 3, 5, 8, 2}; 
  int n = sizeof(arr) / sizeof(arr[0]);
  pid_t pid = fork();

  if (pid < 0) {
    perror("Fork failed");
    return 1;
  } 
  else if (pid == 0) { 
    printf("Child process sorting array...\n");
    qsort(arr, n, sizeof(int), compare);

    printf("Sorted Array in Descending Order: ");
    for (int i = 0; i < n; i++) {
      printf("%d ", arr[i]);
    }
    printf("\n");
    exit(0);
  } 
  else {   
    wait(NULL);
    printf("Parent process checking odd/even status:\n");
    for (int i = 0; i < n; i++) {
      printf("%d is %s\n", arr[i], (arr[i] % 2 == 0) ? "Even" : "Odd");
    }
  }

  return 0;
}
