#include <stdio.h>

#define MAX_SIZE 10
#define OK 0
#define SIZE_INPUT_ERROR 1
#define ARRAY_INPUT_ERROR 2
#define NO_EVEN 3

int input_array(int *const arr, int *const size)
{
    printf("Enter array 1 size\n");
    if (scanf("%d", size) != 1)
        return SIZE_INPUT_ERROR;

    if (!(0 < *size && *size <= MAX_SIZE))
        return SIZE_INPUT_ERROR;

    printf("Enter array 1\n");
    for (int i = 0; i < *size; i++)
        if (scanf("%d", &arr[i]) != 1)
            return ARRAY_INPUT_ERROR;

    return OK;
}

int calc_sum_even(const int *const arr, const int n)
{
    int sum = 0, count_even = 0;

    for (int i = 0; i < n; i++)
    {
        if (arr[i] % 2 == 0)
        {
            sum += arr[i];
            count_even++;
        }
    }

    if (count_even == 0)
        return NO_EVEN;

    return sum;
}

void print_error(const int rc)
{
    if (rc == SIZE_INPUT_ERROR)
        printf("Size input error\n");
    else if (rc == ARRAY_INPUT_ERROR)
        printf("Array input error\n");
    else if (rc == NO_EVEN)
        printf("No even elements in array\n");
}

int main(void)
{
    int arr[MAX_SIZE], size, rc = input_array(arr, &size);

    if (rc == OK)
    {
        int sum = calc_sum_even(arr, size);

        if (sum == NO_EVEN)
            rc = NO_EVEN;
        else
            printf("Sum of even numbers = %d\n", sum);
    }
    
    print_error(rc);
    return rc;
}