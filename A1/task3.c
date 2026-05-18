#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int process_no = 1;

void child_if_odd(pid_t pid){
  if (pid>0 && pid %2 == 1){
    pid_t new_child = fork();
    if(new_child == 0){
      printf("New child for odd PID: %d, My PID: %d, Parent PID: %d\n",pid, getpid(),getppid());
      exit(0);
      
    }else if (new_child > 0){
      process_no++;
      
    }
  }
}
int main() {
  pid_t a, b, c;

  a = fork();
  if (a > 0)
    process_no++; // If fork is successful, increment process count
  child_if_odd(a);

  b = fork();
  if (b > 0)
    process_no++;
  child_if_odd(b);

  c = fork();
  if (c > 0)
    process_no++;
  child_if_odd(c);

  // Wait for all children to finish execution
  while (wait(NULL) > 0)
    ;

  if (getppid() > 1) { // Only parent should print this
    printf("Total processes created: %d\n", process_no);
  }

  return 0;
}
  