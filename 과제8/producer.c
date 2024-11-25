#include "shared.h"

int main() {
    int shm_trigger_id = shmget(SHM_KEY_TRIGGER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int shm_producer_id = shmget(SHM_KEY_PRODUCER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int sem_trigger_id = semget(SEM_KEY_TRIGGER_PRODUCER, 1, 0666 | IPC_CREAT);
    int sem_consumer_id = semget(SEM_KEY_PRODUCER_CONSUMER, 1, 0666 | IPC_CREAT);

    if (shm_trigger_id == -1 || shm_producer_id == -1 || sem_trigger_id == -1 || sem_consumer_id == -1) {
        perror("Failed to create shared memory or semaphore");
        exit(1);
    }

    struct calculation_data *trigger_data = (struct calculation_data *)shmat(shm_trigger_id, NULL, 0);
    struct calculation_data *producer_data = (struct calculation_data *)shmat(shm_producer_id, NULL, 0);

    if (trigger_data == (void *)-1 || producer_data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    int min = INT_MAX, max = INT_MIN;

    while (1) {
        sem_wait(sem_trigger_id); // Trigger 신호 대기

        if (trigger_data->exit_flag) {
            printf("Exiting Producer...\n");
            producer_data->exit_flag = 1;
            sem_signal(sem_consumer_id); // 종료 신호 전달
            break;
        }

        if (!trigger_data->valid_input) {
            continue; // 유효한 입력이 아니면 대기
        }

        // 연산 처리
        int result = 0;
        switch (trigger_data->op) {
            case '+': result = trigger_data->left_num + trigger_data->right_num; break;
            case '-': result = trigger_data->left_num - trigger_data->right_num; break;
            case 'x': result = trigger_data->left_num * trigger_data->right_num; break;
            case '/': 
                result = (trigger_data->right_num != 0) ? trigger_data->left_num / trigger_data->right_num : 0;
                break;
        }

        // 최소, 최대값 업데이트
        if (result < min) min = result;
        if (result > max) max = result;

        // 데이터 복사 및 업데이트
        memcpy(producer_data, trigger_data, sizeof(struct calculation_data));
        producer_data->result = result;
        producer_data->min = min;
        producer_data->max = max;

        // 시간 정보 저장
        time_t now = time(NULL);
        producer_data->timestamp = *localtime(&now);

        // Producer IP 주소 설정
        producer_data->producer_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 로컬 IP 주소 사용

        // Producer에서 1회만 출력하고 대기
        printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
               producer_data->left_num, producer_data->op, producer_data->right_num,
               producer_data->result, producer_data->name, producer_data->min, producer_data->max,
               producer_data->timestamp.tm_hour, producer_data->timestamp.tm_min, producer_data->timestamp.tm_sec,
               inet_ntoa(producer_data->producer_addr.sin_addr));

        // Consumer에 데이터 전달
        sem_signal(sem_consumer_id);

        // Producer의 출력 후 대기
        trigger_data->valid_input = 0; // 유효한 입력 처리 후 대기 상태로 변경
    }

    shmdt(trigger_data);
    shmdt(producer_data);
    shmctl(shm_trigger_id, IPC_RMID, NULL);
    shmctl(shm_producer_id, IPC_RMID, NULL);
    semctl(sem_trigger_id, 0, IPC_RMID);
    semctl(sem_consumer_id, 0, IPC_RMID);

    return 0;
}
