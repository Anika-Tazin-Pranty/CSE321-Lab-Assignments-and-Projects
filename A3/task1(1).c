#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>

struct shared { char sel[2]; int b; };

int main() {
    int id = shmget(99, sizeof(struct shared), 0666 | IPC_CREAT);
    struct shared *d = shmat(id, NULL, 0);
    int fd[2];
    pipe(fd);

    printf("Provide Your Input From Given Options:\n1. Type a to Add Money\n2. Type w to Withdraw Money\n3. Type c to Check Balance\n");
    scanf(" %s", d->sel); d->b = 1000;

    printf("Your selection: %s\n", d->sel);

    if (fork() == 0) {  // child process
        if (strcmp(d->sel, "a") == 0) {
            int amt; printf("Enter amount to be added:\n"); scanf("%d", &amt);
            if (amt > 0) { d->b += amt; printf("Balance added successfully\nUpdated balance after addition: %d\n", d->b); }
            else printf("Adding failed, Invalid amount\n");
        } else if (strcmp(d->sel, "w") == 0) {
            int amt; printf("Enter amount to be withdrawn:\n"); scanf("%d", &amt);
            if (amt > 0 && amt <= d->b) { d->b -= amt; printf("Balance withdrawn successfully\nUpdated balance after withdrawal: %d\n", d->b); }
            else printf("Withdrawal failed, Invalid amount\n");
        } else if (strcmp(d->sel, "c") == 0) printf("Your current balance is: %d\n", d->b);
        else printf("Invalid selection\n");

        write(fd[1], "Thank you for using\n", 21);
        return 0;
    }

    wait(NULL);  // parent process
    char out[50];
    read(fd[0], out, sizeof(out));
    printf("%s", out);

    shmdt(d); shmctl(id, IPC_RMID, NULL);
    return 0;
}
