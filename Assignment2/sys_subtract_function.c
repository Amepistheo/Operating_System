#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

asmlinkage long sys_subtract_function(long num1, long num2, long* sum) {
    long num = num1 - num2;

    copy_to_user(sum, &num, sizeof(long));

    printk("Input: %ld + %ld\n", num1, num2);
    printk("Output: %ld\n", num);

    return 0;
}

SYSCALL_DEFINE3(subtract_function, long, num1, long, num2, long*, sum) {
    return sys_subtract_function(num1, num2, sum);
}
