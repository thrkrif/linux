#include "shared.h"

int main() {
    int shm_id = shmget(SHM_KEY_TRIGGER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int sem_id = semget(SEM_KEY_TRIGGER_PRODUCER, 1, 0666 | IPC_CREAT);

    if (shm_id == -1 || sem_id == -1) {
        perror("Failed to create shared memory or semaphore");
        exit(1);
    }

    // 세마포어 초기값 설정
    semctl(sem_id, 0, SETVAL, 0);

    struct calculation_data *data = (struct calculation_data *)shmat(shm_id, NULL, 0);
    if (data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    data->exit_flag = 0; // 종료 플래그 초기화

    while (1) {
        printf("Enter input (num1 num2 op name) or '0 0 $ quit' to exit: ");
        int ret = scanf("%d %d %c %s", &data->left_num, &data->right_num, &data->op, data->name);

        if (ret != 4) {
            // 잘못된 입력 처리
            printf("Invalid input! Please enter the correct format.\n");
            continue;
        }

        if (data->left_num == 0 && data->right_num == 0 && data->op == '$') {
            printf("Exiting Trigger...\n");
            data->exit_flag = 1;
            sem_signal(sem_id); // 종료 신호 전송
            break;
        }

        if (data->op != '+' && data->op != '-' && data->op != 'x' && data->op != '/') {
            printf("Invalid operator! Please try again.\n");
            continue;
        }

        data->valid_input = 1; // 유효한 입력을 받았다고 표시
        sem_signal(sem_id); // 데이터 준비 완료 신호
    }

    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    return 0;
}
