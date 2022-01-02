// TEST
// std=C99
#include "graph_matrix_visual_print.h"
#include <time.h>

#define PRINT_2D_ARR(fp, arr, row, col, fmt)   \
    do                                         \
    {                                          \
        int _i, _j;                            \
        for (_i = 0; _i < row; ++_i)           \
        {                                      \
            for (_j = 0; _j < col; ++_j)       \
                fprintf(fp, fmt, arr[_i][_j]); \
            fprintf(fp, "\n");                 \
        }                                      \
    } while (0)

int main(int argc, char const *argv[])
{
    // TEST 1
    data_t v_arr[6] = {"Beijing", "Shanghai", "Guangzhou", "Xi'an", "Hongkong", "Chengdu"};
#define INF INFINITE_WEIGHT //为了写起来紧凑
    weight_t matrix[6][6] = {{INF, INF, 9, 3, INF, INF},
                             {INF, 1, INF, INF, 4, INF},
                             {INF, 2, INF, 6, INF, INF},
                             {INF, INF, 5, INF, 2, 3},
                             {1, 4, INF, 2, INF, 6},
                             {5, INF, 7, INF, 6, INF}};
    PRINT_2D_ARR(stdout, matrix, 6, 6, "%4d");
    graph_matrix_visual_print(stdout, 6, v_arr, matrix, "%s", "%u");

    // TEST 2 测试一个随机的
    const int max = 10;
    const int min = 2;
    srand((unsigned int)time(NULL));
    int len = rand() % (max - min + 1) + min;
    int i, j, k;
    data_t v_arr2[len];
    weight_t matrix2[len][len];
    double inf_ratio = 0.4; //无边的个数占所有边个数的比例，不考虑对角线
    size_t inf_count = (size_t)(inf_ratio * (len * len - len));
    size_t count = 0;
    for (i = 0; i < len; i++)
    {
        k = rand() % (max - min + 1) + min;
        v_arr2[i] = (char *)malloc(sizeof(char) * (k + 1));
        for (j = 0; j < k; j++)
            v_arr2[i][j] = 'A' + i;
        v_arr2[i][k] = '\0';
        /////////////////////////////
        for (j = 0; j < len; j++)
        {
            if (i == j || (rand() & 1 && count++ < inf_count)) //上帝掷硬币
            {
                matrix2[i][j] = INFINITE_WEIGHT;
            }
            else
            {
                matrix2[i][j] = 1 + rand() % (65535);
            }
        }
    }

    PRINT_2D_ARR(stdout, matrix2, len, len, "%6d");
    graph_matrix_visual_print(stdout, len, v_arr2, matrix2, "%s", "%u");

    return 0;
}