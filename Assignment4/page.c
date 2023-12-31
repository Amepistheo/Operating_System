#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct Node {
    int page_number;
    int reference_bit;
    struct Node* nextNode;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    int counter;
} Queue;

// Queue 생성 및 초기화
Queue* initQueue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));

    queue->front = queue->rear = NULL;
    queue->counter = 0;

    return queue;
}

// Queue에 page number 추가
void enqueue(Queue* queue, int page_number) {
    Node* newNode = (Node*)malloc(sizeof(Node));

    newNode->page_number = page_number;
    newNode->reference_bit = 0;
    newNode->nextNode = NULL;

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    }
    else {
        queue->rear->nextNode = newNode;
        queue->rear = queue->rear->nextNode;
    }

    queue->counter++;
}

// Queue에 page number 삭제
int dequeue(Queue* queue) {
    // Queue에 값이 없을 경우
    if (queue->front == NULL) {
        return -1; 
    }

    Node* temp = queue->front;
    queue->front = queue->front->nextNode;

    int page_number = temp->page_number;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);

    queue->counter--;

    return page_number;
}

// page number에 해당하는 노드를 Queue의 마지막에 넣어주는 함수
void offerLast(Queue* queue, int page_number) {
    Node* current = queue->front;
    Node* prev = NULL;

    // 이동할 노드를 찾기
    while (current != NULL) {
        if (current->page_number == page_number) {
            if (prev != NULL) {
                prev->nextNode = current->nextNode;
            } 
            // 이동해야하는 노드가 첫번째 노드인 경우 head를 다음 노드로 변경해줌
            else {
                queue->front = current->nextNode;

                if (queue->front == NULL) {
                    queue->rear = NULL;
                }
            }

            // 이동해야하는 노드를 마지막 노드로 변경
            Node* lastNode = queue->front;

            // 노드에 값이 없으면 front와 rear를 동일한 노드에 설정
            if (lastNode == NULL) {
                queue->front = current;
                queue->rear = current;
            } 
            // 큐의 마지막에 이동해야하는 노드 넣기
            else {
                while (lastNode->nextNode != NULL) {
                    lastNode = lastNode->nextNode;
                }

                lastNode->nextNode = current;
            }

            // 마지막 노드가 되었기에 nextNode를 NULL로 설정
            current->nextNode = NULL;
            queue->rear = current; 
            break;
        }

        prev = current;
        current = current->nextNode;
    }
}

// Queue에서 받은 page number에 해당하는 노드의 reference bit를 1로 변경
void changeReferenceBit(Queue* queue, int page_number) {
    Node* current = queue->front;

    // page number에 해당하는 노드 찾기
    while (current != NULL) {
        if (current->page_number == page_number) {
            current->reference_bit = 1;
            break;
        }

        current = current->nextNode;
    }
}

// Reference bit가 0인 첫 노드를 찾아 제거
int rmFirstNodeReferenceBit0(Queue* queue) {
    Node* current = queue->front;
    Node* prev = NULL;

    Queue* movetoLastQueue = initQueue();

    while (current != NULL) {
        if (current->reference_bit != 1) {
            int rm_page_number = current->page_number;

            // 첫번째 노드일 경우
            if (prev == NULL) {
                queue->front = current->nextNode;

                if (queue->front == NULL) {
                    queue->rear = NULL;
                }
            } 
            else {
                prev->nextNode = current->nextNode;

                if (prev->nextNode == NULL) {
                    queue->rear = prev;
                }
            }

            free(current);

            queue->counter--;

            Node* lastNode = movetoLastQueue->front;

            while (lastNode != NULL) {
                offerLast(queue, lastNode->page_number);
                lastNode = lastNode->nextNode;
            }

            // 보조 Queue 메모리 해제
            free(movetoLastQueue); 

            return rm_page_number;
        }
        else {
            current->reference_bit = 0;
            enqueue(movetoLastQueue, current->page_number);
            prev = current;
            current = current->nextNode;
        }
    }

    Node* moveNode = movetoLastQueue->front;

    while (moveNode != NULL) {
        offerLast(queue, moveNode->page_number);
        moveNode = moveNode->nextNode;
    }

    // 보조 Queue 메모리 해제
    free(movetoLastQueue); 

    return dequeue(queue);
}

// Queue에서 Page number에 해당하는 노드를 삭제
void dequeNodeByPageNumber(Queue* queue, int page_number) {
    Node* current = queue->front;
    Node* prev = NULL;

    while (current != NULL) {
        // page number에 해당하는 노드를 찾은 경우
        if (current->page_number == page_number) {
            // 첫 번째 노드일 경우
            if (prev == NULL) {
                queue->front = current->nextNode;

                if (queue->front == NULL) {
                    queue->rear = NULL;
                }
            } 
            else {
                prev->nextNode = current->nextNode;

                if (prev->nextNode == NULL) {
                    queue->rear = prev;
                }
            }

            free(current);

            queue->counter--;

            break;
        }

        prev = current;
        current = current->nextNode;
    }
}

// 가장 나중에 사용될 노드를 찾아서 삭제하고 해당 page number를 return
int rmTheLastNode(Queue* queue, int page_size, int page_table_size, int* page_table, FILE* file) {
    int VR_value;
    int* page_distance = (int*)malloc(page_table_size * sizeof(int));

    // 파일의 시작 지점
    long start_point = ftell(file);

    for (int i = 0; i < page_table_size; i++) {
        page_distance[i] = -1;
    }

    while (fscanf(file, "%d", &VR_value) == 1) {
        int page_index = VR_value / page_size;

        // 처음 사용하는 페이지일 경우
        if (page_distance[page_index] == -1) {
            // 페이지가 다음에 사용되는 위치와 현재 위치와의 거리
            page_distance[page_index] = ftell(file) - start_point;
        }
    }

    // 포인터를 시작 지점으로 원위치
    fseek(file, start_point, SEEK_SET);

    int maximum_distance = -1;
    int rm_page_number = -1;

    Node* current = queue->front;

    while (current != NULL) {
        // 앞으로 페이지가 사용되지 않을 경우
        if (page_distance[current->page_number] == -1) {
            rm_page_number = current->page_number;
            dequeNodeByPageNumber(queue, current->page_number);
            
            return rm_page_number;
        }
        // maximum_distance를 통해 가장 나중에 사용될 page number 찾기
        else {
            if (page_distance[current->page_number] > maximum_distance) {
                maximum_distance = page_distance[current->page_number];
                rm_page_number = current->page_number;
            }
        }

        current = current->nextNode;
    }

    // 큐에서 해당 페이지를 제거
    dequeNodeByPageNumber(queue, rm_page_number);

    free(page_distance);

    return rm_page_number;
}

int customPow(int base, int exponent) {
    int result = 1;

    for (int i = 0; i < exponent; i++) {
        result *= base;
    }

    return result;
}

// 가상주소를 랜덤으로 생성하는 함수
void generateRandomAddresses(FILE* file, int virtual_memory) {
    file = fopen("input.in", "w");

    // 난수 발생을 위한 시드 설정
    srand((unsigned int)time(NULL));

    for (int i = 0; i < 5000; i++) {
        fprintf(file, "%d\n", rand() % (customPow(2, virtual_memory)));
    }

    fclose(file);
}

// Optimal 알고리즘
void simulateOptimal(int page_size, int virtual_memory, int physical_memory, int* page_table, FILE* file, FILE* result_file) {
    int VA_value;
    int PA_value;
    int No_count = 1;

    int frame_number = 0;
    int page_faults = 0;

    Queue* optimalQueue = initQueue();

    fprintf(result_file, "%-8s %-11s %-11s %-11s %-11s %-11s\n", "No.", "V.A.", "Page No.", "Frame No.", "P.A.", "Page Fault");

    while (fscanf(file, "%d", &VA_value) == 1) {
        int page_index = VA_value / page_size;
        int offset = VA_value % page_size;

        // page fault가 발생했을 때
        if (page_table[page_index] == -1) {
            // frame에 값이 모두 할당되지 않았을 때
            if (optimalQueue->counter != (physical_memory / page_size)) {
                frame_number = optimalQueue->counter;
                page_table[page_index] = frame_number;
                enqueue(optimalQueue, page_index);

                PA_value = frame_number * page_size + offset;

                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }
            // 모든 frame에 값이 할당됐을 때
            else {
                int page_table_size = customPow(2, virtual_memory) / page_size;
                int removed_page = rmTheLastNode(optimalQueue, page_size, page_table_size, page_table, file);
                
                frame_number = page_table[removed_page];
                page_table[removed_page] = -1;
                page_table[page_index] = frame_number;
                enqueue(optimalQueue, page_index);

                PA_value = frame_number * page_size + offset;

                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }

            page_faults++;
        }
        // Hit일 경우
        else {
            frame_number = page_table[page_index];
            PA_value = frame_number * page_size + offset;

            fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                    No_count++, VA_value, page_index, frame_number, PA_value, 'H');
        }
    }

    fprintf(result_file, "Total Number of Page Faults: %d\n", page_faults);
}


// FIFO 알고리즘
void simulateFIFO(int page_size, int physical_memory, int* page_table, FILE* file, FILE* result_file) {
    int VA_value;
    int PA_value;
    int No_count = 1;
    
    int frame_number = 0;
    int page_faults = 0;

    Queue* fifoQueue = initQueue();

    fprintf(result_file, "%-8s %-11s %-11s %-11s %-11s %-11s\n", "No.", "V.A.", "Page No.", "Frame No.", "P.A.", "Page Fault");

    while (fscanf(file, "%d", &VA_value) == 1) {
        int page_index = VA_value / page_size;
        int offset = VA_value % page_size;

        // page fault가 발생했을 때
        if (page_table[page_index] == -1) {
            // frame에 값이 모두 할당되지 않았을 때
            if (fifoQueue->counter != (physical_memory / page_size)) {
                frame_number = fifoQueue->counter;
                page_table[page_index] = frame_number;
                enqueue(fifoQueue, page_index);

                PA_value = frame_number * page_size + offset;
                
                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }
            // 모든 frame에 값이 할당됐을 때
            else {
                int replaced_page = dequeue(fifoQueue);

                frame_number = page_table[replaced_page];
                page_table[replaced_page] = -1;
                page_table[page_index] = frame_number;
                enqueue(fifoQueue, page_index);

                PA_value = frame_number * page_size + offset;
                
                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }

            page_faults++;
        } 
        // Hit일 경우
        else {
            frame_number = page_table[page_index];
            PA_value = frame_number * page_size + offset;

            fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                    No_count++, VA_value, page_index, frame_number, PA_value, 'H');
        }
    }

    fprintf(result_file, "Total Number of Page Faults: %d\n", page_faults);
}

// LRU 알고리즘
void simulateLRU(int page_size, int physical_memory, int* page_table, FILE* file, FILE* result_file) {
    int VA_value;
    int PA_value;
    int No_count = 1;
    
    int frame_number = 0;
    int page_faults = 0;

    Queue* lruQueue = initQueue();

    fprintf(result_file, "%-8s %-11s %-11s %-11s %-11s %-11s\n", "No.", "V.A.", "Page No.", "Frame No.", "P.A.", "Page Fault");

    while (fscanf(file, "%d", &VA_value) == 1) {
        int page_index = VA_value / page_size;
        int offset = VA_value % page_size;

        // page fault가 발생했을 때
        if (page_table[page_index] == -1) {
            // frame에 값이 모두 할당되지 않았을 때
            if (lruQueue->counter != (physical_memory / page_size)) {
                frame_number = lruQueue->counter;
                page_table[page_index] = frame_number;
                enqueue(lruQueue, page_index);

                PA_value = frame_number * page_size + offset;

                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }
            // 모든 frame에 값이 할당됐을 때
            else {
                int removed_page = dequeue(lruQueue);

                frame_number = page_table[removed_page];
                page_table[removed_page] = -1;
                page_table[page_index] = frame_number;
                enqueue(lruQueue, page_index);

                PA_value = frame_number * page_size + offset;

                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }

            page_faults++;
        }
        // Hit일 경우
        else {
            frame_number = page_table[page_index];
            PA_value = frame_number * page_size + offset;

            // 해당 노드를 Queue의 마지막에 넣어줌
            offerLast(lruQueue, page_index);

            fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                    No_count++, VA_value, page_index, frame_number, PA_value, 'H');
        }
    }

    fprintf(result_file, "Total Number of Page Faults: %d\n", page_faults);
}

// Second-Chance (One-handed Clock) 알고리즘
void simulateSC(int page_size, int physical_memory, int* page_table, FILE* file, FILE* result_file) {
    int VA_value;
    int PA_value;
    int No_count = 1;
    
    int frame_number = 0;
    int page_faults = 0;

    Queue* ScQueue = initQueue();

    fprintf(result_file, "%-8s %-11s %-11s %-11s %-11s %-11s\n", "No.", "V.A.", "Page No.", "Frame No.", "P.A.", "Page Fault");

    while (fscanf(file, "%d", &VA_value) == 1) {
        int page_index = VA_value / page_size;
        int offset = VA_value % page_size;

        // page fault가 발생했을 때
        if (page_table[page_index] == -1) {
            // frame에 값이 모두 할당되지 않았을 때
            if (ScQueue->counter != (physical_memory / page_size)) {
                frame_number = ScQueue->counter;
                page_table[page_index] = frame_number;
                enqueue(ScQueue, page_index);

                PA_value = frame_number * page_size + offset;

                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }
            // 모든 frame에 값이 할당됐을 때
            else {
                int removed_page = rmFirstNodeReferenceBit0(ScQueue);

                frame_number = page_table[removed_page];
                page_table[removed_page] = -1;
                page_table[page_index] = frame_number;
                enqueue(ScQueue, page_index);

                PA_value = frame_number * page_size + offset;

                fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                        No_count++, VA_value, page_index, frame_number, PA_value, 'F');
            }

            page_faults++;
        }
        // Hit일 경우
        else {
            frame_number = page_table[page_index];
            PA_value = frame_number * page_size + offset;

            // second chance를 주기 위해 해당 노드의 reference bit를 1로 변경
            changeReferenceBit(ScQueue, page_index);

            fprintf(result_file, "%-8d %-11d %-11d %-11d %-11d %c\n",
                    No_count++, VA_value, page_index, frame_number, PA_value, 'H');
        }
    }

    fprintf(result_file, "Total Number of Page Faults: %d\n", page_faults);
}

int main() {
    // 문자열을 입력받을 버퍼
    char input[256]; 
    int virtual_memory = 0;
    int page_size = 0;
    int physical_memory = 0;
    int PR = 0;
    int vm_string = 0;
    FILE* file;
    FILE* result_file;

    // 가상주소 길이
    while(1) {
        printf("A. Simulation에 사용할 가상주소 길이를 선택하시오 (1. 18bits  2. 19bits  3. 20bits): ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // input의 길이가 1개인지를 판별하는 변수
            int n = sscanf(input, "%d", &virtual_memory);

            if (n == 1 && virtual_memory >= 1 && virtual_memory <= 3) {
                break;
            }
        }
    }

    if (virtual_memory == 1) {
        virtual_memory = 18;
    }
    else if (virtual_memory == 2) {
        virtual_memory = 19;
    }
    else if (virtual_memory == 3) {
        virtual_memory = 20;
    }

    printf("\n");

    // 페이지(프레임)의 크기
    while(1) {
        printf("B. Simulation에 사용할 페이지(프레임)의 크기를 선택하시오 (1. 1KB  2. 2KB  3. 4KB): ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // input의 길이가 1개인지를 판별하는 변수
            int n = sscanf(input, "%d", &page_size);

            if (n == 1 && page_size >= 1 && page_size <= 3) {
                break;
            }
        }
    }

    if (page_size == 1) {
        page_size = customPow(2, 10);
    }
    else if (page_size == 2) {
        page_size = customPow(2, 11);
    }
    else if (page_size == 3) {
        page_size = customPow(2, 12);
    }

    printf("\n");

    // 물리메모리의 크기
    while(1) {
        printf("C. Simulation에 사용할 물리메모리의 크기를 선택하시오 (1. 32KB  2. 64KB): ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // input의 길이가 1개인지를 판별하는 변수
            int n = sscanf(input, "%d", &physical_memory);

            if (n == 1 && physical_memory >= 1 && physical_memory <= 2) {
                break;
            }
        }
    }

    if (physical_memory == 1) {
        physical_memory = customPow(2, 15);
    }
    else if (physical_memory == 2) {
        physical_memory = customPow(2, 16);
    }

    printf("\n");

    // 알고리즘 선택
    while(1) {
        printf("D. Simulation에 적용할 Page Replacement 알고리즘을 선택하시오\n(1. Optimal  2. FIFO  3. LRU  4. Second-Chance): ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // input의 길이가 1개인지를 판별하는 변수
            int n = sscanf(input, "%d", &PR);

            if (n == 1 && PR >= 1 && PR <= 4) {
                break;
            }
        }
    }

    printf("\n");

    // 가상주소 스트링 입력방식
    while(1) {
        printf("E. 가상주소 스트링 입력방식을 선택하시오\n(1. input.in 자동 생성  2. 기존 방식 파일 사용): ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // input의 길이가 1개인지를 판별하는 변수
            int n = sscanf(input, "%d", &vm_string);

            if (n == 1) {
                // input.in 자동 생성
                if (vm_string == 1) {
                    generateRandomAddresses(file, virtual_memory);
                    file = fopen("input.in", "r");
                    break;
                } 
                // 기존 방식 파일 사용
                else if (vm_string == 2) {
                    while (1) {
                        printf("\nF. 입력 파일 이름을 입력하시오: ");

                        fgets(input, sizeof(input), stdin);
                        // 입력값의 마지막 '\n' 제거
                        input[strcspn(input, "\n")] = '\0';

                        // 입력값에 공백이 있는지 확인
                        if (strspn(input, " ") == strlen(input)) {
                            printf("Error: 입력값에 공백이 있습니다.\n");
                            continue;  // 루프의 처음으로 이동하여 다시 입력 받음
                        }

                        // 읽기 모드로 파일 열기
                        file = fopen(input, "r");
                        
                        // 파일이 열리면 break, 아니라면 Error 출력
                        if (file == NULL) {
                            printf("Error: 파일이 존재하지 않습니다.\n");
                        } 
                        else {
                            break;
                        }
                    }
               }
            }

            break;
        }
    }

    int page_count = customPow(2, virtual_memory) / page_size;

    // (가상주소의 길이 / 페이지의 크기 = 페이지의 개수)를 통해 페이지 테이블 생성
    int* page_table = (int*)malloc(page_count * sizeof(int));

    // Page Table의 값을 -1로 초기화
    // Page Table에 frame number가 들어가 있고, -1이라면 page fault 발생
    for (int i=0; i<page_count; i++) {
        page_table[i] = -1;
    }

    // Optimal
    if (PR == 1) {
        result_file = fopen("output.opt", "w");
        if (result_file == NULL) {
            perror("Error: 파일을 여는데 실패하였습니다.\n");
            return EXIT_FAILURE;
        }

        simulateOptimal(page_size, virtual_memory, physical_memory, page_table, file, result_file);

        fclose(result_file);
    }

    // FIFO
    else if (PR == 2) {
        result_file = fopen("output.fifo", "w");
        if (result_file == NULL) {
            perror("Error: 파일을 여는데 실패하였습니다.\n");
            return EXIT_FAILURE;
        }

        simulateFIFO(page_size, physical_memory, page_table, file, result_file);

        fclose(result_file);
    }

    // LRU
    else if (PR == 3) {
        result_file = fopen("output.lru", "w");
        if (result_file == NULL) {
            perror("Error: 파일을 여는데 실패하였습니다.\n");
            return EXIT_FAILURE;
        }

        simulateLRU(page_size, physical_memory, page_table, file, result_file);

        fclose(result_file);
    }

    // Second-Chance(One-handed Clock)
    else if (PR == 4) {
        result_file = fopen("output.sc", "w");
        if (result_file == NULL) {
            perror("Error: 파일을 여는데 실패하였습니다.\n");
            return EXIT_FAILURE;
        }

        simulateSC(page_size, physical_memory, page_table, file, result_file);

        fclose(result_file);
    }

    free(page_table);
}
