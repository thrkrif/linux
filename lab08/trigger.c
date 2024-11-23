#include "shared.h"

int main() {
    int shm_id = shmget(SHM_KEY_TRIGGER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Failed to create shared memory");
        exit(1);
    }

    struct calculation_data *data = (struct calculation_data *)shmat(shm_id, NULL, 0);
    if (data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    while (1) {
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
        scanf("%d %d %c %s", &data->left_num, &data->right_num, &data->op, data->name);

        if (data->left_num == 0 && data->right_num == 0 && data->op == '$') {
            printf("Exiting Trigger...\n");
            break;
        }
    }

    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}
