#include "shared.h"

int main() {
    int shm_id = shmget(SHM_KEY_PRODUCER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
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
        printf("%d %c %d = %d %s min=%d max=%d %s %d-%d-%d %02d:%02d:%02d from %s\n",
               data->left_num, data->op, data->right_num,
               data->result, data->name,
               data->min, data->max,
               "Thu", data->timestamp.tm_year + 1900, data->timestamp.tm_mon + 1, data->timestamp.tm_mday,
               data->timestamp.tm_hour, data->timestamp.tm_min, data->timestamp.tm_sec,
               inet_ntoa(data->producer_addr.sin_addr));

        sleep(10);
    }

    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}
