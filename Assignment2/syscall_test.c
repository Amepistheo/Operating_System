#include <stdio.h>
#include <ctype.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

enum { NUM, ADD, SUBTRACT, ERROR, END } current_token;

char* number;
long length = 0;

// 버퍼를 비우는 역할
void flush() {
    char c;
    while ((c = getchar()) != '\n');
}

// 에러 출력하는 역할
void error() {
    printf("Wrong Input!\n");
}

// 토큰을 얻는 함수
void token_parse() {
    char c;

    // 공백 문자 건너뛰기
    while ((c = getchar()) == ' ' || c == '\t');

    if (c == '\n') {
        current_token = END;
        return;
    } 
    else if (c == '+') {
        current_token = ADD;
        return;
    } 
    else if (c == '-') {
        current_token = SUBTRACT;
        return;
    } 
    else if (isdigit(c)) {
        // 숫자인 경우
        char num[1024] = { c };
        int index = 1;

        // 일의 자리 숫자가 아닌 경우 계속 입력받아 num 배열에 넣음
        while (isdigit(c = getchar())) {
            num[index++] = c;
        }

        num[index] = '\0';
        length = index;

        // 숫자가 한 자리가 아닌 경우 getchar()를 계속 실행하고 마지막으로 입력 받은 문자를 stdin에 다시 넣음
        ungetc(c, stdin);

        // stdin을 통해 다시 넣은 수를 number에 저장
        number = num;

        current_token = NUM;
        return;
    }
    else {
        // 잘못된 입력인 경우
        current_token = ERROR;
        return;
    }
}

int main() {

    while (1) {
        printf("Input : ");

        // 처음 토큰 가져옴
        token_parse();                    

        // 첫 토큰이 숫자일 경우
        if (current_token == NUM) {

            //첫번째 숫자
            char num1[1024];
            sprintf(num1, "%s", number);

            // 토큰 읽어옴 (+, -, \n)
            token_parse();

            // 토큰이 END일 경우 -> sys_reverse_digits 호출
            if (current_token == END) {
                
                // 입력받은 값을 역순으로 출력하는 system call 호출
                syscall(450, number, length);
                printf("Output : %s\n", number);
            }

            // 토큰이 '-'일 경우 -> sys_add_function 호출
            else if (current_token == SUBTRACT) {

                // 토큰 가져옴
                token_parse();

                // 토큰이 숫자일 경우
                if (current_token == NUM) {

                    // 두번째 숫자
                    long num2 = atoi(number);
                    // 연산 결과
                    long sum;

                    token_parse();
                    
                    //토큰이 END인 경우 덧셈 system call 호출
                    if (current_token == END) {
                        syscall(451, atoi(num1), num2, &sum);
                        printf("Output : %ld\n", sum);
                    }
                    else {
                        flush();
                        error();
                    }
                }

                // 토큰이 숫자가 아닌 경우 ERROR 처리
                else {
                    if(current_token != END) {
                        flush();
                    }

                    error();
                }
            }

            // 토큰이 '+'일 경우 -> sys_subtract_function 호출
            else if (current_token == ADD) {

                // 토큰 가져옴
                token_parse();

                // 토큰이 숫자일 경우
                if (current_token == NUM) {

                    // 두번째 숫자
                    long num2 = atoi(number);
                    // 연산 결과
                    long sum;

                    token_parse();
                    
                    //토큰이 END인 경우 뺄셈 system call 호출
                    if (current_token == END) {
                        syscall(452, atoi(num1), num2, &sum);
                        printf("Output : %ld\n", sum);
                    }
                    else {
                        flush();
                        error();
                    }
                }

                // 다음 토큰이 숫자가 아닌 경우 ERROR 처리
                else {
                    if (current_token != END) {
                        flush();
                    }

                    error();
                }
            }

            // 읽어 온 토큰이 (+, -, \n)이 아닌 경우 ERROR 처리 후 flush()를 통해 버퍼 비움
            else {
                error();
                flush();
            } 
        }

        // 이외의 모든 경우는 ERROR 처리 및 버퍼 비우기
        else {
            if (current_token == END) {
                break;
            }
            else {
                error();
                flush();
            }
        }
    }
}
