#include "shared.h"

int main() {
    int shm_id = shmget(SHM_KEY_PRODUCER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int sem_id = semget(SEM_KEY_PRODUCER_CONSUMER, 1, 0666 | IPC_CREAT);

    if (shm_id == -1 || sem_id == -1) {
        perror("Failed to create shared memory or semaphore");
        exit(1);
    }

    struct calculation_data *data = (struct calculation_data *)shmat(shm_id, NULL, 0);
    if (data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    struct calculation_data last_data = {0}; // 이전 데이터를 저장할 변수
    int data_changed = 0; // 데이터가 변경되었는지 체크하는 플래그

    while (1) {
        sem_wait(sem_id); // Producer 신호 대기

        if (data->exit_flag) {
            printf("Exiting Consumer...\n");
            break;  // 종료 플래그가 세팅되면 종료
        }

        if (!data->valid_input) {
            continue; // 유효한 입력이 아니면 대기
        }

        // 데이터가 변경된 경우에만 출력 시작
        if (memcmp(data, &last_data, sizeof(struct calculation_data)) != 0) {
            // 데이터가 변경되었을 때 출력 준비
            memcpy(&last_data, data, sizeof(struct calculation_data)); // 데이터를 갱신
            data_changed = 1; // 데이터 변경 상태로 설정
        }

        // 데이터가 변경되었으면 10초마다 반복 출력
        if (data_changed) {
            while (1) {  // 데이터가 변경되었으면 반복적으로 출력
                time_t now = time(NULL);
                struct tm *current_time = localtime(&now);

                printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
                    data->left_num, data->op, data->right_num, data->result,
                    data->name, data->min, data->max,
                    current_time->tm_hour, current_time->tm_min, current_time->tm_sec,
                    inet_ntoa(data->producer_addr.sin_addr));

                sleep(10); // 10초 간격으로 출력

                // 종료 상태를 체크해서 종료
                if (data->exit_flag) {
                    printf("Exiting Consumer due to trigger and producer termination...\n");
                    break;
                }
            }
        }
    }

    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}
