#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h> // Include for strcpy
#include "mpi.h"

void printParenthesis(int i, int j, int n, int bracket[][n], char* name)
{
    if (i == j)
    {
        printf("%c", (*name)++);
        return;
    }

    printf("(");
    printParenthesis(i, bracket[i][j], n, bracket, name);
    printParenthesis(bracket[i][j] + 1, j, n, bracket, name);
    printf(")");
}

int main(int argc, char **argv)
{
    int rank;
    MPI_Init(&argc, &argv);

    int array[7] = {0};
    int count = 0;
    int i = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
        char myText[100];
        char lines[7][100];
        FILE *file = fopen("dimensions.txt", "r");

        if (file == NULL)
        {
            printf("Error opening the file.\n");
            MPI_Finalize();
            return 1;
        }

        while (i < 7 && fgets(myText, sizeof(myText), file))
        {
            strcpy(lines[i], myText);
            i++;
            count++;
        }

        fclose(file);

        i = 0;
        while (i < count)
        {
            int j = i;
            if (i < count - 1)
            {
                sscanf(lines[i], "%d", &array[j]);
            }
            else
            {
                sscanf(lines[i], "%d", &array[j++]);
                sscanf(lines[i] + 5, "%d", &array[j]);
                break;
            }
            i++;
        }

        count++;
        MPI_Send(&count, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(array, count, MPI_INT, 1, 1, MPI_COMM_WORLD);
    }

    if (rank == 1)
    {
        int count;
        MPI_Recv(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int p[count];

        MPI_Recv(p, count, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int n = count;
        int m[n][n];
        int bracket[n][n];

        for (int i = 1; i < n; i++)
            m[i][i] = 0;

        for (int L = 2; L < n; L++)
        {
            for (int i = 1; i < n - L + 1; i++)
            {
                int j = i + L - 1;
                m[i][j] = INT_MAX;
                for (int k = i; k <= j - 1; k++)
                {
                    int q = m[i][k] + m[k + 1][j] + p[i - 1] * p[k] * p[j];
                    if (q < m[i][j])
                    {
                        m[i][j] = q;
                        bracket[i][j] = k;
                    }
                }
            }
        }

        char name = 'A';
// Optimal Parenthesization
	printf("\n\nOptimized Order of Multiplication: ");
        printParenthesis(1, n - 1, n, bracket, &name);
        printf("\nCost of Optimized Order Multiplication:  %d\n", m[1][n - 1]);
        name = 'A';
        FILE *outputFile = freopen("cost.txt", "w", stdout);
        if (outputFile == NULL)
        {
            printf("Failed to open cost.txt for writing.\n");
            MPI_Finalize();
            return 1;
        }

        // Optimal Parenthesization
        printParenthesis(1, n - 1, n, bracket, &name);
        printf(" %d\n", m[1][n - 1]);

        fclose(outputFile);
    }

    MPI_Finalize();

    return 0;
}
