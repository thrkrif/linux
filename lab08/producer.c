#include "shared.h"

int main() {
    int shm_trigger_id = shmget(SHM_KEY_TRIGGER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int shm_producer_id = shmget(SHM_KEY_PRODUCER, sizeof(struct calculation_data), 0666 | IPC_CREAT);

    if (shm_trigger_id == -1 || shm_producer_id == -1) {
        perror("Failed to create shared memory");
        exit(1);
    }

    struct calculation_data *trigger_data = (struct calculation_data *)shmat(shm_trigger_id, NULL, 0);
    struct calculation_data *producer_data = (struct calculation_data *)shmat(shm_producer_id, NULL, 0);

    if (trigger_data == (void *)-1 || producer_data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    int min = INT_MAX, max = INT_MIN;
    struct sockaddr_in producer_addr;
    producer_addr.sin_family = AF_INET;
    producer_addr.sin_addr.s_addr = inet_addr("10.20.0.123");

    while (1) {
        if (trigger_data->left_num == 0 && trigger_data->right_num == 0 && trigger_data->op == '$') {
            printf("Exiting Producer...\n");
            break;
        }

        int result = 0;
        switch (trigger_data->op) {
            case '+': result = trigger_data->left_num + trigger_data->right_num; break;
            case '-': result = trigger_data->left_num - trigger_data->right_num; break;
            case 'x': result = trigger_data->left_num * trigger_data->right_num; break;
            case '/': 
                if (trigger_data->right_num == 0) result = 0; 
                else result = trigger_data->left_num / trigger_data->right_num; 
                break;
            default: 
                continue;
        }

        if (result < min) min = result;
        if (result > max) max = result;

        memcpy(producer_data, trigger_data, sizeof(struct calculation_data));
        producer_data->result = result;
        producer_data->min = min;
        producer_data->max = max;
        time_t now = time(NULL);
        producer_data->timestamp = *localtime(&now);
        producer_data->producer_addr = producer_addr;

        sleep(10);
    }

    shmdt(trigger_data);
    shmdt(producer_data);
    shmctl(shm_trigger_id, IPC_RMID, NULL);
    shmctl(shm_producer_id, IPC_RMID, NULL);
    return 0;
}
