#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// 선언부
DIR *dir;
struct dirent *entry;
FILE* file;

int main(int argc, char** argv) {
    
    int pid, self_tty, file_directory;
    unsigned long user_time, kernel_time, total_time;
    char isProcess, *tty, status;
    char cmd[1024], tty_cur[1024], path[1024], time[1024];

    // 현재 실행중인 TTY의 name 저장
    dir = opendir("/proc");
    self_tty = open("/proc/self/fd/0", O_RDONLY);
    sprintf(tty_cur, "%s", ttyname(self_tty));

    printf("%5s %s\t%8s %s\n", "PID", "TTY", "TIME", "CMD");

    // dir의 모든 디렉터리 탐색
    while ((entry = readdir(dir)) != NULL) {
        // 기본 값은 1 
        isProcess = 1;

        for (int i = 0; entry->d_name[i]; i++) {
            // entry가 프로세스가 아닌경우 다음 루프로 넘어감 (프로세스는 숫자)
            if (!isdigit(entry->d_name[i])) {
                isProcess = 0;
                break;
            }
        }
	    
        // 프로세스인 경우
        if (isProcess) {
            // TTY 찾기
            sprintf(path, "/proc/%s/fd/0", entry->d_name);
            file_directory = open(path, O_RDONLY);
            tty = ttyname(file_directory);

            // 위의 TTY와 현재 실행중인 프로세스의 TTY가 같은 경우
            if (tty && strcmp(tty, tty_cur) == 0) {
                // CMD 이름 저장
                sprintf(path, "/proc/%s/stat", entry->d_name);
                file = fopen(path, "r");
                fscanf(file, "%d %s %*c %c %*c", &pid, cmd, &status);
                cmd[strlen(cmd) - 1] = '\0';

                int i = 0;

                // user mode 시간
                while (i<11) {
                    fscanf(file, "%lu", &user_time);
                    i++;
                }

                // kernel mode 시간
                fscanf(file, "%lu", &kernel_time);

                // 최종 시간 초 단위로 계산
                total_time = (int)((double)(user_time + kernel_time) / sysconf(_SC_CLK_TCK));
                sprintf(time, "%02lu:%02lu:%02lu", (total_time / 3600) % 3600, (total_time / 60) % 60, total_time % 60);

                printf("%5s %s\t%8s %s\n", entry->d_name, tty + 5, time, cmd + 1);

                fclose(file);
            }

            close(file_directory);
        }
    }

    close(self_tty);

    return 0;
}
