// #include "shared.h"

// void producer(int client_id) {
//     int shm_trigger_id = shmget(SHM_KEY_TRIGGER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
//     int shm_producer_id = shmget(SHM_KEY_PRODUCER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
//     int sem_trigger_id = semget(SEM_KEY_TRIGGER_PRODUCER, 1, 0666 | IPC_CREAT);
//     int sem_consumer_id = semget(SEM_KEY_PRODUCER_CONSUMER, 1, 0666 | IPC_CREAT);

//     if (shm_trigger_id == -1 || shm_producer_id == -1 || sem_trigger_id == -1 || sem_consumer_id == -1) {
//         perror("Failed to create shared memory or semaphore");
//         exit(1);
//     }

//     struct calculation_data *trigger_data = (struct calculation_data *)shmat(shm_trigger_id, NULL, 0);
//     struct calculation_data *producer_data = (struct calculation_data *)shmat(shm_producer_id, NULL, 0);

//     if (trigger_data == (void *)-1 || producer_data == (void *)-1) {
//         perror("Failed to attach shared memory");
//         exit(1);
//     }

//     int min = INT_MAX, max = INT_MIN;

//     while (1) {
//         sem_wait(sem_trigger_id); // Trigger 신호 대기

//         if (trigger_data->exit_flag) {
//             producer_data->exit_flag = 1;
//             sem_signal(sem_consumer_id); // 종료 신호 전달
//             break;
//         }

//         if (!trigger_data->valid_input) {
//             continue;
//         }

//         // 연산 처리
//         int result = 0;
//         switch (trigger_data->op) {
//             case '+': result = trigger_data->left_num + trigger_data->right_num; break;
//             case '-': result = trigger_data->left_num - trigger_data->right_num; break;
//             case 'x': result = trigger_data->left_num * trigger_data->right_num; break;
//             case '/': result = (trigger_data->right_num != 0) ? trigger_data->left_num / trigger_data->right_num : 0; break;
//         }

//         // 최소, 최대값 업데이트
//         if (result < min) min = result;
//         if (result > max) max = result;

//         // 데이터 복사 및 업데이트
//         memcpy(producer_data, trigger_data, sizeof(struct calculation_data));
//         producer_data->result = result;
//         producer_data->min = min;
//         producer_data->max = max;

//         // Producer 주소 설정
//         producer_data->producer_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

//         sem_signal(sem_consumer_id); // Consumer로 데이터 전달
//     }

//     shmdt(trigger_data);
//     shmdt(producer_data);
// }

// void consumer(int client_id) {
//     int shm_producer_id = shmget(SHM_KEY_PRODUCER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
//     int sem_consumer_id = semget(SEM_KEY_PRODUCER_CONSUMER, 1, 0666 | IPC_CREAT);

//     if (shm_producer_id == -1 || sem_consumer_id == -1) {
//         perror("Failed to create shared memory or semaphore");
//         exit(1);
//     }

//     struct calculation_data *data = (struct calculation_data *)shmat(shm_producer_id, NULL, 0);
//     if (data == (void *)-1) {
//         perror("Failed to attach shared memory");
//         exit(1);
//     }

//     while (1) {
//         sem_wait(sem_consumer_id); // Producer 신호 대기

//         if (data->exit_flag) {
//             break; // 종료 플래그가 설정되면 종료
//         }

//         // 출력 반복
//         while (1) {
//             time_t now = time(NULL);
//             struct tm *current_time = localtime(&now);

//             printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
//                    data->left_num, data->op, data->right_num, data->result,
//                    data->name, data->min, data->max,
//                    current_time->tm_hour, current_time->tm_min, current_time->tm_sec,
//                    inet_ntoa(data->producer_addr.sin_addr));

//             sleep(10); // 10초 간격 출력

//             if (data->exit_flag) {
//                 break;
//             }
//         }
//     }

//     shmdt(data);
// }

// int main() {
//     int client_id = getpid(); // 클라이언트 고유 ID
//     pid_t pid = fork();

//     if (pid == 0) {
//         // 자식 프로세스: Consumer 실행
//         consumer(client_id);
//     } else {
//         // 부모 프로세스: Producer 실행
//         producer(client_id);
//     }

//     return 0;
// }

#include "shared.h"

void producer(int client_id) {
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

    while (1) {
        sem_wait(sem_trigger_id); // Trigger 신호 대기

        if (trigger_data->exit_flag) {
            producer_data->exit_flag = 1;
            sem_signal(sem_consumer_id); // 종료 신호 전달
            break;
        }

        if (!trigger_data->valid_input) {
            continue;
        }

        // 연산 처리
        int result = 0;
        switch (trigger_data->op) {
            case '+': result = trigger_data->left_num + trigger_data->right_num; break;
            case '-': result = trigger_data->left_num - trigger_data->right_num; break;
            case 'x': result = trigger_data->left_num * trigger_data->right_num; break;
            case '/': result = (trigger_data->right_num != 0) ? trigger_data->left_num / trigger_data->right_num : 0; break;
        }

        // 클라이언트별 최소, 최대값 관리
        if (trigger_data->min > result || trigger_data->min == INT_MAX) trigger_data->min = result;
        if (trigger_data->max < result || trigger_data->max == INT_MIN) trigger_data->max = result;

        // 데이터 복사 및 업데이트
        memcpy(producer_data, trigger_data, sizeof(struct calculation_data));
        producer_data->result = result;
        producer_data->min = trigger_data->min;
        producer_data->max = trigger_data->max;

        // Producer 주소 설정
        producer_data->producer_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        sem_signal(sem_consumer_id); // Consumer로 데이터 전달
    }

    shmdt(trigger_data);
    shmdt(producer_data);
}

void consumer(int client_id) {
    int shm_producer_id = shmget(SHM_KEY_PRODUCER, sizeof(struct calculation_data), 0666 | IPC_CREAT);
    int sem_consumer_id = semget(SEM_KEY_PRODUCER_CONSUMER, 1, 0666 | IPC_CREAT);

    if (shm_producer_id == -1 || sem_consumer_id == -1) {
        perror("Failed to create shared memory or semaphore");
        exit(1);
    }

    struct calculation_data *data = (struct calculation_data *)shmat(shm_producer_id, NULL, 0);
    if (data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    while (1) {
        sem_wait(sem_consumer_id); // Producer 신호 대기

        if (data->exit_flag) {
            break; // 종료 플래그가 설정되면 종료
        }

        // 출력 반복
        while (1) {
            time_t now = time(NULL);
            struct tm *current_time = localtime(&now);

            printf("%d %c %d = %d %s min=%d max=%d %02d:%02d:%02d from %s\n",
                   data->left_num, data->op, data->right_num, data->result,
                   data->name, data->min, data->max,
                   current_time->tm_hour, current_time->tm_min, current_time->tm_sec,
                   inet_ntoa(data->producer_addr.sin_addr));

            sleep(10); // 10초 간격 출력

            if (data->exit_flag) {
                break;
            }
        }
    }

    shmdt(data);
}

int main() {
    int client_id = getpid(); // 클라이언트 고유 ID
    pid_t pid = fork();

    if (pid == 0) {
        // 자식 프로세스: Consumer 실행
        consumer(client_id);
    } else {
        // 부모 프로세스: Producer 실행
        producer(client_id);
    }

    return 0;
}
