// std=C99
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef char *data_t;

typedef unsigned int weight_t;
#define INFINITE_WEIGHT -1

#ifndef DEBUG_DYNAMIC_MEM_CHECK
#define DEBUG_DYNAMIC_MEM_CHECK
#endif

#ifdef DEBUG_DYNAMIC_MEM_CHECK
static size_t g_alloc_count = 0; //全局变量
static size_t g_free_count = 0;
#endif

static void my_free(void *p)
{
    free(p);
#ifdef DEBUG_DYNAMIC_MEM_CHECK
    g_free_count++;
#endif
}

static void *my_alloc(size_t size)
{
    void *p = malloc(size);
#ifdef DEBUG_DYNAMIC_MEM_CHECK
    g_alloc_count++;
#endif
    return p;
}

/*
int is_symmetry(size_t elem_num, const weight_t (*const matrix)[elem_num])
{
    int i, j, flag = 0;
    for (i = 1; i < elem_num && flag == 0; ++i)
        for (j = 0; j < i && flag == 0; ++j)
            if (matrix[i][j] != matrix[j][i])
                flag = 1;
    return flag == 0;
}
*/

//因为静态求总高度是很困难的, 至少现在看来是, 所以用链式
typedef struct _Line
{
    struct _Line *up;
    char *line;
    int h;
    struct _Line *down;
} Line;

//还是写个函数吧，省地方。
/**
 * @brief 创建一个“行”结构体。例如top = new_line_link(20, top, 1, ' ');
 *
 * @param width 行宽
 * @param cur 当前结构体指针，目的是既要创建，又要将当前的与新建的连接
 * @param top_or_bottom 大于0表示cur为top，小于0表示cur为bottom. 等于0表示不需要连接，无视cur，仅用于初始状态下创建一个mid
 * @param fill 用来填充的字符串
 * @return Line* 新建的指针。推荐的做法是将此返回值赋值给原来的top或bottom. (毕竟这里cur不是**)
 */
Line *new_line_link(size_t width, Line *cur, int top_or_bottom, char fill)
{
    Line *p = (Line *)my_alloc(sizeof(Line));
    p->line = (char *)my_alloc(sizeof(char) * (width + 1)); //  \0
    memset(p->line, fill, width);
    p->line[width] = '\0';
    if (top_or_bottom > 0)
    {
        p->h = cur->h + 1;
        p->up = NULL;
        p->down = cur;
        cur->up = p;
    }
    else if (top_or_bottom < 0)
    {
        p->h = cur->h - 1;
        p->down = NULL;
        p->up = cur;
        cur->down = p;
    }
    else
    {
        p->h = 0;
        p->down = NULL;
        p->up = NULL;
    }
    return p;
}

void graph_matrix_visual_print(FILE *fp, const size_t elem_num, const data_t *const v_arr, const weight_t (*const matrix)[elem_num], const char *fmt, const char *weight_fmt_str)
{ //不考虑自己和自己，不区分有向图无向图，一律看作有向图

    const size_t interval = 3; //初始默认元素间隔
    const size_t margin = 1;   //边上权值到两边的dash数量 例如 /-----65535-----\ 这个margin就是5, /-3-\这个就是1
    const char up_right = '/';
    const char down_right = '\\';
    const char up_left = '\\';
    const char down_left = '/';
    const char dash = '-';
    const char vertical_dash = '|';
    const char cross = '~'; /* 当两个线相交但是不是真正相交时，跨过去，类似于电路图中带弯的线 */
    const char left_bracket = '[';
    const char right_bracket = ']';
    const char up_arrow = '^';
    const char down_arrow = 'v';
    // const size_t inner_edge_height = 2;  //最内层边的高度，为了方便，是2
    // const size_t edge_height_differ = 1; //同一顶点两边高度之差
    const size_t MAX_ELEM_STR_LEN = 128;

    int i, j, k, m;
    if (elem_num < 1)
        return;
    if (elem_num < 2)
    {
        fprintf(fp, "%c", left_bracket);
        fprintf(fp, fmt, v_arr[0]);
        fprintf(fp, "%c", right_bracket);
        return;
    }

    // 计算元素那行的宽度
    char elem_str_arr[elem_num][MAX_ELEM_STR_LEN];
    size_t elem_len_arr[elem_num];
    size_t interval_arr[elem_num - 1]; // 0~1, 1~2, ..., n-2~n-1
    size_t gross_width = 0;
    char weight_str_temp[MAX_ELEM_STR_LEN];
    char weight_str_temp2[MAX_ELEM_STR_LEN];

    //如果要在边上打印权值, 那么权值很大的时候, 数字长度可能超过线的长度,
    //所以需要预先求出权值的数字字符串长度, 然后调整间隔
    //这是一个艰难的过程
    //因为只有当两个点的边的权值大于在一维展开时中间有所元素长度和间隔之差才需要调整
    //而且往往是下标相邻的点最可能得到调整
    //然而剩下的情况,先判断哪个也许是有讲究的
    //现在考虑先看相邻, 再看隔一个, 再看隔两个... 越往后调整的概率越小
    //这相当于对于矩阵来说, 斜线顺序遍历, 主对角线,主对角线长度减一的对角线, 减二的...
    //  n^2 = \frac{2 \times [1+(n-1)](n-1)}{2}
    for (i = 0; i < elem_num; i++)
    {
        sprintf(elem_str_arr[i], fmt, v_arr[i]); // TODO:
        elem_len_arr[i] = strlen(elem_str_arr[i]);
    }
    for (i = 0; i < elem_num - 1; i++)
        interval_arr[i] = interval;
    size_t w_len;        //权值的str的宽度
    size_t sum_len;      //累加一维状态下两点之间所有str的长度和, 不算括号
    size_t sum_intv_len; //累加一维状态下两点之间元素仅interval的长度和
    for (i = 1; i <= elem_num - 1; ++i)
    { // i就是步长
        for (j = 0; j + i <= elem_num - 1; j++)
        {
            if (!(matrix[j][j + i] == INFINITE_WEIGHT && matrix[j + i][j] == INFINITE_WEIGHT))
            {
                if (matrix[j][j + i] != INFINITE_WEIGHT)
                    sprintf(weight_str_temp, weight_fmt_str, matrix[j][j + i]);
                if (matrix[j + i][j] != INFINITE_WEIGHT)
                    sprintf(weight_str_temp2, weight_fmt_str, matrix[j + i][j]);
                if (matrix[j][j + i] != INFINITE_WEIGHT && matrix[j + i][j] != INFINITE_WEIGHT)
                    w_len = strlen(weight_str_temp2) > strlen(weight_str_temp) ? strlen(weight_str_temp2) : strlen(weight_str_temp);
                else if (matrix[j][j + i] != INFINITE_WEIGHT)
                    w_len = strlen(weight_str_temp);
                else
                    w_len = strlen(weight_str_temp2);

                for (k = j + 1, sum_len = interval_arr[j], sum_intv_len = sum_len; k < j + i; k++)
                {
                    sum_len += elem_len_arr[k] + interval_arr[k];
                    sum_intv_len += interval_arr[k];
                }

                while (sum_len < w_len + 2 * margin || sum_intv_len % i != 0)
                {
                    sum_len++;
                    sum_intv_len++;
                }
                //利用margin 扩充, 暂时采用均匀扩充方法, 也许有更好的方式, 交给上帝吧
                //先向上增加到能整除? 还是除完向上取整? 一样的

                for (k = 0; k < i; ++k)
                    interval_arr[j + k] = sum_intv_len / i;
            }
        }
    }

    for (i = 0; i < elem_num; ++i) //求总宽
        gross_width += i < elem_num - 1 ? elem_len_arr[i] + interval_arr[i] + 2 : elem_len_arr[i] + 2;

    Line *mid = new_line_link(gross_width, NULL, 0, ' ');
    Line *top = mid;
    Line *bottom = mid;
    top = new_line_link(gross_width, mid, 1, ' ');
    bottom = new_line_link(gross_width, mid, -1, ' ');

    //求左右括号的下标 , 并且将这三行填充
    size_t left_bra_arr[elem_num];
    size_t right_bra_arr[elem_num];
    for (i = 0, k = 0; i < elem_num; ++i)
    {
        left_bra_arr[i] = k;
        mid->line[k++] = left_bracket;

        for (j = 0; j < strlen(elem_str_arr[i]); ++j)
            mid->line[k + j] = elem_str_arr[i][j]; // copy
        k += strlen(elem_str_arr[i]);

        right_bra_arr[i] = k;
        mid->line[k++] = right_bracket;

        for (j = 0; j < interval_arr[i]; ++j, ++k)
            ; //走过interval

        for (j = i + 1; j < elem_num; ++j)
            if (matrix[i][j] != INFINITE_WEIGHT)
                break; //上三角有向后出的
            else
                continue;
        if (j < elem_num)
            top->line[right_bra_arr[i]] = vertical_dash;

        for (j = 0; j < i; ++j)
            if (matrix[j][i] != INFINITE_WEIGHT)
                break; //上三角，即比i小的元素有流入i的，即看i的入度
            else
                continue;
        if (j < i)
            top->line[left_bra_arr[i]] = down_arrow;

        for (j = 0; j < i; ++j)
            if (matrix[i][j] != INFINITE_WEIGHT)
                break; //下三角，考察比i大的元素导致的i的出度
            else
                continue;
        if (j < i)
            bottom->line[left_bra_arr[i]] = vertical_dash;

        for (j = i + 1; j < elem_num; ++j)
            if (matrix[j][i] != INFINITE_WEIGHT)
                break; //下三角，考察比i大的元素导致的i的入度
            else
                continue;
        if (j < elem_num)
            bottom->line[right_bra_arr[i]] = up_arrow;
    }

    for (i = elem_num - 2; i >= 0; --i) //某个点所有向后的都考虑完，再下一个。
    {                                   //即n-2所有向后的，n-3所有向后的..., 1所有向后的，0所有向后的
        // for (j = elem_num - 1; j >= i + 1; --j)
        for (j = i + 1; j <= elem_num - 1; ++j) //先考虑近的，再考虑远的
        {
            if (matrix[i][j] == INFINITE_WEIGHT)
                continue; //这个放在for中写是不对的
            sprintf(weight_str_temp, weight_fmt_str, matrix[i][j]);
            if (top->h == 1) // 只有一层，显然直接用
            {
                top = new_line_link(gross_width, top, 1, ' ');
                top->line[right_bra_arr[i]] = up_right;
                for (k = right_bra_arr[i] + 1; k <= right_bra_arr[i] + (left_bra_arr[j] - right_bra_arr[i] - 1 - strlen(weight_str_temp)) / 2; k++)
                    top->line[k] = dash;
                for (m = 0; m < strlen(weight_str_temp); m++)
                    top->line[k + m] = weight_str_temp[m];
                for (k += strlen(weight_str_temp); k < left_bra_arr[j]; k++)
                    top->line[k] = dash;
                top->line[left_bra_arr[j]] = down_right;
            }
            else
            { // top不在1层，但是仍然有可能在2层开始画， 而不是从top所在开始画
                Line *p = mid->up->up;
                int flag;
                do
                {
                    flag = 0;
                    for (k = right_bra_arr[i] + 1; k < left_bra_arr[j]; ++k) //
                        if (p->line[k] != ' ')                               //从后往前画线,那么横线部分永远是优先,优先意味着必须是空的
                        {
                            flag = 1;
                            break;
                        }
                    if (flag)
                    {
                        if (p->up != NULL)
                        {
                            p = p->up; //一直向上走
                        }
                        else
                        {
                            break;
                        }
                    }

                } while (flag != 0);
                if (flag)
                {
                    top = new_line_link(gross_width, top, 1, ' ');
                    top->line[right_bra_arr[i]] = up_right;
                    for (k = right_bra_arr[i] + 1; k <= right_bra_arr[i] + (left_bra_arr[j] - right_bra_arr[i] - 1 - strlen(weight_str_temp)) / 2; k++)
                        top->line[k] = dash;
                    for (m = 0; m < strlen(weight_str_temp); m++)
                        top->line[k + m] = weight_str_temp[m];
                    for (k += strlen(weight_str_temp); k < left_bra_arr[j]; k++)
                        top->line[k] = dash;
                    top->line[left_bra_arr[j]] = down_right;
                }
                else
                {
                    p->line[right_bra_arr[i]] = up_right;
                    for (k = right_bra_arr[i] + 1; k <= right_bra_arr[i] + (left_bra_arr[j] - right_bra_arr[i] - 1 - strlen(weight_str_temp)) / 2; k++)
                        p->line[k] = dash;
                    for (m = 0; m < strlen(weight_str_temp); m++)
                        p->line[k + m] = weight_str_temp[m];
                    for (k += strlen(weight_str_temp); k < left_bra_arr[j]; k++)
                        p->line[k] = dash;
                    p->line[left_bra_arr[j]] = down_right;
                }

                //接下来补充竖线
                Line *q;
                for (q = mid->up->up; q != (flag ? top : p); q = q->up)
                {
                    if (q->line[right_bra_arr[i]] == ' ')
                        q->line[right_bra_arr[i]] = vertical_dash;

                    if (q->line[left_bra_arr[j]] == ' ')
                        q->line[left_bra_arr[j]] = vertical_dash;
                    else if (q->line[left_bra_arr[j]] == dash)
                        q->line[left_bra_arr[j]] = cross;
                }
            } // else 非2层

        } // for

    } // for

    //下半部， 与上同理。为了仍然i出j入，做了i，j互换
    for (j = elem_num - 2; j >= 0; --j) //某个点所有向后的都考虑完，再下一个。
    {                                   //即n-2所有向后的，n-3所有向后的..., 1所有向后的，0所有向后的
        // for (j = elem_num - 1; j >= i + 1; --j)
        for (i = j + 1; i <= elem_num - 1; ++i) //先考虑近的，再考虑远的
        {
            if (matrix[i][j] == INFINITE_WEIGHT)
                continue; //这个放在for中写是不对的
            sprintf(weight_str_temp, weight_fmt_str, matrix[i][j]);
            if (bottom->h == -1) // 只有一层，显然直接用
            {
                bottom = new_line_link(gross_width, bottom, -1, ' ');
                bottom->line[right_bra_arr[j]] = up_left;
                for (k = right_bra_arr[j] + 1; k <= right_bra_arr[j] + (left_bra_arr[i] - right_bra_arr[j] - 1 - strlen(weight_str_temp)) / 2; k++)
                    bottom->line[k] = dash;
                for (m = 0; m < strlen(weight_str_temp); m++)
                    bottom->line[k + m] = weight_str_temp[m];
                for (k += strlen(weight_str_temp); k < left_bra_arr[i]; k++)
                    bottom->line[k] = dash;
                bottom->line[left_bra_arr[i]] = down_left;
            }
            else
            { // bottom不在1层，但是仍然有可能在2层开始画， 而不是从bottom所在开始画
                Line *p = mid->down->down;
                int flag;
                do
                {
                    flag = 0;
                    for (k = right_bra_arr[j] + 1; k < left_bra_arr[i]; ++k) //
                        if (p->line[k] != ' ')                               //从后往前画线,那么横线部分永远是优先,优先意味着必须是空的
                        {
                            flag = 1;
                            break;
                        }
                    if (flag)
                    {
                        if (p->down != NULL)
                        {
                            p = p->down; //一直向上走
                        }
                        else
                        {
                            break;
                        }
                    }

                } while (flag != 0);
                if (flag)
                {
                    bottom = new_line_link(gross_width, bottom, -1, ' ');
                    bottom->line[right_bra_arr[j]] = up_left;
                    for (k = right_bra_arr[j] + 1; k <= right_bra_arr[j] + (left_bra_arr[i] - right_bra_arr[j] - 1 - strlen(weight_str_temp)) / 2; k++)
                        bottom->line[k] = dash;
                    for (m = 0; m < strlen(weight_str_temp); m++)
                        bottom->line[k + m] = weight_str_temp[m];
                    for (k += strlen(weight_str_temp); k < left_bra_arr[i]; k++)
                        bottom->line[k] = dash;
                    bottom->line[left_bra_arr[i]] = down_left;
                }
                else
                {
                    p->line[right_bra_arr[j]] = up_left;
                    for (k = right_bra_arr[j] + 1; k <= right_bra_arr[j] + (left_bra_arr[i] - right_bra_arr[j] - 1 - strlen(weight_str_temp)) / 2; k++)
                        p->line[k] = dash;
                    for (m = 0; m < strlen(weight_str_temp); m++)
                        p->line[k + m] = weight_str_temp[m];
                    for (k += strlen(weight_str_temp); k < left_bra_arr[i]; k++)
                        p->line[k] = dash;
                    p->line[left_bra_arr[i]] = down_left;
                }

                //接下来补充竖线
                Line *q;
                for (q = mid->down->down; q != (flag ? bottom : p); q = q->down)
                {
                    if (q->line[right_bra_arr[j]] == ' ')
                        q->line[right_bra_arr[j]] = vertical_dash;

                    if (q->line[left_bra_arr[i]] == ' ')
                        q->line[left_bra_arr[i]] = vertical_dash;
                    else if (q->line[left_bra_arr[i]] == dash)
                        q->line[left_bra_arr[i]] = cross;
                }
            } // else 非2层

        } // for

    } // for

    Line *out, *f; //输出
    for (out = top; out != NULL; out = out->down)
    {
        fprintf(fp, "%s", out->line);
        fprintf(fp, "\n");
    }

    for (out = top; out != NULL; out = f)
    { //释放
        f = out->down;
        my_free(out->line);
        my_free(out);
    }

#ifdef DEBUG_DYNAMIC_MEM_CHECK
    fprintf(stderr, "DEBUG_DYNAMIC_MEM_CHECK: in file %s, line: %d, data: %s, time: %s, g_alloc_count: %u, g_free_count: %u.\n", __FILE__, __LINE__, __DATE__, __TIME__, g_alloc_count, g_free_count);
#endif
}
