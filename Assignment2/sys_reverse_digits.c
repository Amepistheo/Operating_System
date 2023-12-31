#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/string.h>

asmlinkage long sys_reverse_digits(char __user *user_input, long input_len) {
    
    char input_k[1024];			// kernel space에 저장할 데이터를 담을 버퍼
    char reverse_str[1024];		// input의 역순 결과를 저장하는 버퍼
    
    // 입력 데이터의 유효성 검사
    if (!user_input || input_len >= 1024) {
        return -1; 			// 유효하지 않은 인수 또는 길이 초과
    }
    
    // user space에서 kernel space로 데이터 복사
    if (copy_from_user(input_k, user_input, input_len) < 0) {
        return -1; 			// 복사 실패
    }

    input_k[input_len] = '\0'; 		// 문자열 끝

   int i = 0;

    // input을 거꾸로 변환
    for (i = 0; i < input_len; i++) {
        reverse_str[i] = input_k[input_len - i - 1];
    }

    reverse_str[input_len] = '\0';	// 문자열 끝

    copy_to_user(user_input, reverse_str, input_len);	

    // reverse_str 출력
    printk("Input: %s\n", input_k);
    printk("Output: %s\n", reverse_str);

    return 0;
}

SYSCALL_DEFINE2(reverse_digits, char __user *, user_input, long, input_len) {
    return sys_reverse_digits(user_input, input_len);
}
