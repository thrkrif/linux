#include "shared.h"

int main() {
    int shm_trigger_id = shmget(SHM_KEY_TRIGGER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int sem_trigger_id = semget(SEM_KEY_TRIGGER_PRODUCER, 1, 0666 | IPC_CREAT);

    if (shm_trigger_id == -1 || sem_trigger_id == -1) {
        perror("Failed to create shared memory or semaphore");
        exit(1);
    }

    // 세마포어 초기값 설정
    semctl(sem_trigger_id, 0, SETVAL, 0);

    struct calculation_data *trigger_data = (struct calculation_data *)shmat(shm_trigger_id, NULL, 0);
    if (trigger_data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    trigger_data->exit_flag = 0; // 종료 플래그 초기화

    printf("Server started. Waiting for input...\n");

    while (1) {
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
        int ret = scanf("%d %d %c %s", &trigger_data->left_num, &trigger_data->right_num, &trigger_data->op, trigger_data->name);

        if (ret != 4) {
            printf("Invalid input! Please enter the correct format.\n");
            continue;
        }

        if (trigger_data->left_num == 0 && trigger_data->right_num == 0 && trigger_data->op == '$') {
            printf("Exiting Server...\n");
            trigger_data->exit_flag = 1;
            sem_signal(sem_trigger_id); // 종료 신호 전송
            break;
        }

        if (trigger_data->op != '+' && trigger_data->op != '-' && trigger_data->op != 'x' && trigger_data->op != '/') {
            printf("Invalid operator! Please try again.\n");
            continue;
        }

        trigger_data->valid_input = 1; // 유효한 입력
        sem_signal(sem_trigger_id); // 트리거 준비 완료 신호
    }

    shmdt(trigger_data);
    shmctl(shm_trigger_id, IPC_RMID, NULL);
    semctl(sem_trigger_id, 0, IPC_RMID);

    return 0;
} 