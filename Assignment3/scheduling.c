#define _GNU_SOURCE  // sched_setaffinity 함수를 사용하기 위해 필요한 매크로
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
#include <locale.h>

#define N 100
#define PROCESS_NUM 21

void formatTime(struct timeval currentTime, char* formattedTime) {
    // 한국 시간 지역 설정 설정
    setlocale(LC_TIME, "ko_KR.UTF-8");
    
    // 초 단위 시간을 변환
    time_t timeInSeconds = currentTime.tv_sec;
    struct tm* timeInfo = localtime(&timeInSeconds);

    // 마이크로초를 포함한 시간 문자열로 변환
    strftime(formattedTime, 30, "%H:%M:%S", timeInfo);
    sprintf(formattedTime + 8, ".%06ld", currentTime.tv_usec);
}

void child_process() {
    int matrixA[N][N] = {0};
    int matrixB[N][N] = {0};
    int result[N][N] = {0};

    int count = 0; 
    int k, i, j;

    while(count < 100) {
        for(k = 0; k < 100; k++) {
            for(i = 0; i < 100; i++) {
                for(j = 0; j < 100; j++) {
                    result[k][j] += matrixA[k][i] * matrixB[i][j];
                }
            }
        }

        count++;
    }
}

int main() {
    // 문자열을 입력받을 버퍼
    char input[256]; 
    int pid;
    int scheduling_num = 0;
    int time_quantum = 0;
    int child_pipe[PROCESS_NUM][2];
    struct timeval start, end;
    double elapsed_time, total_execution_time = 0;

    // 스케줄링 정책 결정
    while(1) {
        printf("Input the Scheduling Policy to apply:\n");
        printf("1. CFS_DEFAULT\n");
        printf("2. CFS_NICE\n");
        printf("3. RT_FIFO\n");
        printf("4. RT_RR\n");
        printf("0. Exit\n");

        if (fgets(input, sizeof(input), stdin) != NULL) {
            // input의 길이가 1개인지를 판별하는 변수
            int n = sscanf(input, "%d", &scheduling_num);

            if (n == 1 && scheduling_num >= 0 && scheduling_num <= 4) {
                break;
            }
        }
    }

    if (scheduling_num == 3) {
        // 초기화 sched_param 구조체
        struct sched_param param;
        memset(&param, 0, sizeof(struct sched_param));

        param.sched_priority = 1;

        // RT_FIFO로 정책 변경
        int result = sched_setscheduler(getpid(), SCHED_FIFO, &param);

        if (result == -1) {
            perror("Error sched_setscheduler");
            exit(EXIT_FAILURE);
        }

    }

    else if (scheduling_num == 4) {
        while(1) {
            printf("Select Time Quantum (1: 10, 2: 100, 3: 1000): ");
            
            if (fgets(input, sizeof(input), stdin) != NULL) {
                // input의 길이가 1개인지를 판별하는 변수
                int n = sscanf(input, "%d", &time_quantum);

                if (n == 1 && time_quantum >= 1 && time_quantum <= 3) {
                    break;
                }
            }
        }

        // '/proc/sys/kernel/sched_rr_timeslice_ms' 파일을 쓰기 모드로 열기
        FILE *file = fopen("/proc/sys/kernel/sched_rr_timeslice_ms", "w");

        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        // 입력받은 시간 퀀텀 값을 파일에 쓰기
        if (time_quantum == 1) {
            fprintf(file, "%d", 10);
        }  
        else if (time_quantum == 2) {
            fprintf(file, "%d", 100);
        }
        else if (time_quantum == 3) {
            fprintf(file, "%d", 1000);
        } 

        // 파일 닫기
        fclose(file);

        // 초기화 sched_param 구조체
        struct sched_param param;
        memset(&param, 0, sizeof(struct sched_param));

        param.sched_priority = 1;

        // RT_RR로 정책 변경
        int result = sched_setscheduler(getpid(), SCHED_RR, &param);

        if (result == -1) {
            perror("Error sched_setscheduler");
            exit(EXIT_FAILURE);
        }
    }

    //CPU core를 하나로 제한
    cpu_set_t set_core;
    // CPU 초기화
    CPU_ZERO(&set_core);  
    // CPU 코어 0을 사용하도록 설정         
    CPU_SET(0, &set_core);         

    // 0번 core로 설정이 불가하면 error
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set_core) == -1) {
       perror("Error sched_setaffinity");
       exit(EXIT_FAILURE);
    }

    for (int i=0; i<PROCESS_NUM; i++) {
        // 파이프 생성
        if (pipe(child_pipe[i]) == -1) {
            perror("Error pipe");
            exit(EXIT_FAILURE);
        }
        
        // 시작 시간
        gettimeofday(&start, NULL); 

        // 자식 프로세스 생성
        pid_t pid = fork();

        // 프로세스 오류
        if (pid < 0) {
            perror("Error fork");
            exit(EXIT_FAILURE);
        }

        // 자식 프로세스인 경우
        else if (pid == 0) {

            // 자식 프로세스에서 읽기 파이프 닫기
            close(child_pipe[i][0]);
            
            // 1. CFS_DEFAULT
            if (scheduling_num == 1) {

                int nice = getpriority(PRIO_PROCESS, 0);

                char start_time[30];
                formatTime(start, start_time);

                // 자식 프로세스 실행
                child_process();
                
                // 종료 시간
                gettimeofday(&end, NULL);
                char end_time[30];
                formatTime(end, end_time);

                elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

                printf("PID: %d | NICE: %d | Start time: %s | End time: %s | Elapsed time: %.6f\n", getpid(), nice, start_time, end_time, elapsed_time);

                // 메시지를 출력 파이프를 통해 부모 프로세스에게 보내기
                write(child_pipe[i][1], &elapsed_time, sizeof(double));
                // 파이프의 쓰기 닫기
                close(child_pipe[i][1]); 

                exit(EXIT_SUCCESS);
            }
            // 2. CFS_NICE
            else if (scheduling_num == 2) {
                int nice;

                if(i < 7){
                    nice = 19;
                }
                else if(i < 14){
                    nice = 0;
                }
                else{
                    nice = -20;
                }

                if (setpriority(PRIO_PROCESS, 0, nice) == -1) {
                    perror("Error setpriority");
                    exit(EXIT_FAILURE);
                }

                char start_time[30];
                formatTime(start, start_time);

                // 자식 프로세스 실행
                child_process();
                
                // 종료 시간
                gettimeofday(&end, NULL);
                char end_time[30];
                formatTime(end, end_time);

                elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

                printf("PID: %d | NICE: %d | Start time: %s | End time: %s | Elapsed time: %.6f\n", getpid(), nice, start_time, end_time, elapsed_time);

                // 메시지를  출력 파이프를 통해 부모 프로세스에게 보내기
                write(child_pipe[i][1], &elapsed_time, sizeof(double));
                // 파이프의 쓰기 닫기
                close(child_pipe[i][1]); 

                exit(EXIT_SUCCESS);
            }
            // 3. RT_FIFO
            else if (scheduling_num == 3) {
                char start_time[30];
                formatTime(start, start_time);

                // 자식 프로세스 실행
                child_process();
                
                // 종료 시간
                gettimeofday(&end, NULL);
                char end_time[30];
                formatTime(end, end_time);

                elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

                printf("PID: %d | Start time: %s | End time: %s | Elapsed time: %.6f\n", getpid(), start_time, end_time, elapsed_time);

                // 메시지를  출력 파이프를 통해 부모 프로세스에게 보내기
                write(child_pipe[i][1], &elapsed_time, sizeof(double));
                // 파이프의 쓰기 닫기
                close(child_pipe[i][1]); 

                exit(EXIT_SUCCESS);
            }
            // 4. RT_RR
            else if (scheduling_num == 4) {
                char start_time[30];
                formatTime(start, start_time);

                // 자식 프로세스 실행
                child_process();
                
                // 종료 시간
                gettimeofday(&end, NULL);
                char end_time[30];
                formatTime(end, end_time);

                elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

                printf("PID: %d | Start time: %s | End time: %s | Elapsed time: %.6f\n", getpid(), start_time, end_time, elapsed_time);

                // 메시지를  출력 파이프를 통해 부모 프로세스에게 보내기
                write(child_pipe[i][1], &elapsed_time, sizeof(double));
                // 파이프의 쓰기 닫기
                close(child_pipe[i][1]); 

                exit(EXIT_SUCCESS);
            }
            // 5. EXIT
            else if (scheduling_num == 0) {
                exit(0);
            }

            exit(0);
        }
    }

    // 자식 프로세스의 종료를 대기
    for (int i=0; i<PROCESS_NUM; i++) {
        wait(NULL);
    }

    // 부모 프로세스에서 쓰기 파이프 닫기
    for (int i=0; i<PROCESS_NUM; i++) {
        close(child_pipe[i][1]);
    }

    // 부모 프로세스에서 자식 프로세스로부터 시간 받아와 평균 시간 계산
    for (int i=0; i<PROCESS_NUM; i++) {
        double execution_time;
        read(child_pipe[i][0], &execution_time, sizeof(double));
        total_execution_time += execution_time;
        close(child_pipe[i][0]); 
    }

    double average_execution_time = total_execution_time / PROCESS_NUM;

    if(scheduling_num == 1) {
        printf("Scheduling Policy: CFS_DEFAULT | Average elapsed time: %.6f\n", average_execution_time);
    } 
    else if(scheduling_num == 2) {
        printf("Scheduling Policy: CFS_NICE | Average elapsed time: %.6f\n", average_execution_time);
    }
    else if(scheduling_num == 3) {
        printf("Scheduling Policy: RT_FIFO | Average elapsed time: %.6f\n", average_execution_time);
    }
    else if(scheduling_num == 4) {
        FILE *file;
        int time_slice;

        // 읽기 모드로 파일 열기
        file = fopen("/proc/sys/kernel/sched_rr_timeslice_ms", "r");

        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        // 파일에서 값을 읽어오기
        fscanf(file, "%d", &time_slice);

        // 파일 닫기
        fclose(file);

        printf("Scheduling Policy: RT_RR | Time Quantum: %d ms | Average elapsed time: %.6f\n", time_slice, average_execution_time);

        // time_slice 값을 기본값인 100으로 다시 변경 
        file = fopen("/proc/sys/kernel/sched_rr_timeslice_ms", "w");

        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        // 새로운 시간 퀀텀 값을 파일에 쓰기
        fprintf(file, "%d", 100);

        // 파일 닫기
        fclose(file);
    }

    return 0;
}
