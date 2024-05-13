#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define MAX_STACK_SIZE 10

typedef struct {
    double** arr;
    int row, col;
} Matrix;

Matrix stack[MAX_STACK_SIZE];
int stackIndex = -1;

void stackPush(Matrix m) {
    if (stackIndex < MAX_STACK_SIZE - 1) {
        stack[++stackIndex] = m;
    }
}

void stackPop() {
    if (stackIndex > -1) {
        stackIndex--;
    }
}

Matrix stackTop() {
    return stackIndex > -1 ? stack[stackIndex] : (Matrix) { .row = 0, .col = 0, .arr = NULL };
}

void loadMatrixDimensions(const char* filename, int** rowSizes, int** colSizes, int* matrixCount) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open dimensions file");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    *matrixCount = 0;
    while (fgets(buffer, sizeof(buffer), file)) {
        (*matrixCount)++;
    }

    rewind(file);

    *rowSizes = (int*)malloc(*matrixCount * sizeof(int));
    *colSizes = (int*)malloc(*matrixCount * sizeof(int));

    int i = 0;
    while (i < *matrixCount) {
        fscanf(file, "%d %d", &(*rowSizes)[i], &(*colSizes)[i]);
        i++;
    }

    fclose(file);
}

void generateRandomMatrices(Matrix* matrices, int matrixCount, int* rows, int* columns) {
    srand(time(0));

    int i = 0;
    while (i < matrixCount) {
        matrices[i].row = rows[i];
        matrices[i].col = columns[i];

        matrices[i].arr = (double**)malloc(rows[i] * sizeof(double*));

        int a = 0;
        while (a < rows[i]) {
            matrices[i].arr[a] = (double*)malloc(columns[i] * sizeof(double));
            a++;
        }

        int b = 0;
        while (b < matrices[i].row) {
            int c = 0;
            while (c < matrices[i].col) {
                matrices[i].arr[b][c] = rand() % 10;
                c++;
            }
            b++;
        }

        i++;
    }
}

void printMatrix(Matrix matrix) {
    int a = 0;
    while (a < matrix.row) {
        int b = 0;
        while (b < matrix.col) {
            printf("%f  ", matrix.arr[a][b]);
            b++;
        }
        printf("\n");
        a++;
    }
    printf("\n");
}

void workerNode(Matrix* matrices, int size, int rank) {
    MPI_Status status;
    MPI_Request request;

    while (1) {
        int source = 0;
        int offset = 0, row1 = 0, col1 = 0, row2 = 0, col2 = 0;

        MPI_Irecv(&offset, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        if (offset == -1) {
            // Termination signal received
            break;
        }

        MPI_Irecv(&row1, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        MPI_Irecv(&col1, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        MPI_Irecv(&row2, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        MPI_Irecv(&col2, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        double** matrixA = malloc(row1 * sizeof(double*));
        int a = 0;
        while (a < row1) {
            matrixA[a] = malloc(col1 * sizeof(double));
            a++;
        }

        double** matrixB = malloc(row2 * sizeof(double*));
        int b = 0;
        while (b < row2) {
            matrixB[b] = malloc(col2 * sizeof(double));
            b++;
        }

        MPI_Irecv(&matrixA[0][0], row1 * col1, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        MPI_Irecv(&matrixB[0][0], row2 * col2, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);

        // Perform matrix multiplication
        double** resultmat = malloc(row1 * sizeof(double*));
        int c = 0;
        while (c < row1) {
            resultmat[c] = malloc(col2 * sizeof(double));
            c++;
        }

        int k = 0;
        while (k < col2) {
            int i = 0;
            while (i < row1) {
                resultmat[i][k] = 0.0;
                int j = 0;
                while (j < col1) {
                    resultmat[i][k] += matrixA[i][j] * matrixB[j][k];
                    j++;
                }
                i++;
            }
            k++;
        }

        // Send results back to the root
        MPI_Isend(&offset, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &request);
        MPI_Isend(&row1, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &request);
        MPI_Isend(&resultmat[0][0], row1 * col2, MPI_DOUBLE, source, 2, MPI_COMM_WORLD, &request);

        // Free allocated memory
        int d = 0;
        while (d < row1) {
            free(matrixA[d]);
            d++;
        }
        free(matrixA);

        int e = 0;
        while (e < row2) {
            free(matrixB[e]);
            e++;
        }
        free(matrixB);

        int f = 0;
        while (f < row1) {
            free(resultmat[f]);
            f++;
        }
        free(resultmat);
    }
}

int main(int argc, char* argv[]) {
    struct timeval begin, end;
    gettimeofday(&begin, 0);

    int matrixCount = 0;
    int* rows;
    int* columns;
    int root = 0;

    MPI_Status status;

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Matrix matrices[matrixCount];
    if (rank == root) {
        char filename[] = "dimensions.txt";
        loadMatrixDimensions(filename, &rows, &columns, &matrixCount);
        Matrix matrices[matrixCount];
        generateRandomMatrices(matrices, matrixCount, rows, columns);

        int i = 0;
        while (i < matrixCount) {
            printf("Matrix : %d \n", i + 1);
            printMatrix(matrices[i]);
            i++;
        }

        // Perform matrix multiplication
        char order[50];
        int cost;
        FILE* file = fopen("cost.txt", "r");

        if (file == NULL) {
            printf("Error opening the file.\n");
            return 1;
        }

        int readresult = fscanf(file, "%49s %d", order, &cost);
        if (readresult == 2) {
            printf("Read from file: Order = %s, Cost = %d\n", order, cost);
        } else {
            printf("Failed to read from file.\n");
        }

        fclose(file);
        printf("Order: %s \n", order);

        // Perform matrix multiplication based on the order
        double** resultMatrix = matrices[order[0] - 'A'].arr;

        int j = 1;
        while (j < matrixCount) {
            int currentMatrixIndex = order[j] - 'A';
            int previousMatrixIndex = order[j - 1] - 'A';

            // Perform matrix multiplication
            double** currentMatrix = matrices[currentMatrixIndex].arr;
            double** previousResultMatrix = resultMatrix;

            int row1 = matrices[previousMatrixIndex].row;
            int col1 = matrices[previousMatrixIndex].col;
            int row2 = matrices[currentMatrixIndex].row;
            int col2 = matrices[currentMatrixIndex].col;

            double** tempResult = (double**)malloc(row1 * sizeof(double*));
            int a = 0;
            while (a < row1) {
                tempResult[a] = (double*)malloc(col2 * sizeof(double));
                a++;
            }

            int k = 0;
            while (k < col2) {
                int i = 0;
                while (i < row1) {
                    tempResult[i][k] = 0.0;
                    int j = 0;
                    while (j < col1) {
                        tempResult[i][k] += previousResultMatrix[i][j] * currentMatrix[j][k];
                        j++;
                    }
                    i++;
                }
                k++;
            }

            // Free previous result matrix
            int a = 0;
            while (a < row1) {
                free(previousResultMatrix[a]);
                a++;
            }
            free(previousResultMatrix);

            // Set the new result matrix
            resultMatrix = tempResult;
            j++;
        }

        // Display the final result matrix
        printf("Result Matrix \n");
        int a = 0;
        while (a < matrices[order[matrixCount - 1] - 'A'].row) {
            int b = 0;
            while (b < matrices[order[matrixCount - 1] - 'A'].col) {
                printf("%f  ", resultMatrix[a][b]);
                b++;
            }
            printf("\n");
            a++;
        }

        // Free allocated memory
        int i = 0;
        while (i < matrixCount) {
            int j = 0;
            while (j < matrices[i].row) {
                free(matrices[i].arr[j]);
                j++;
            }
            free(matrices[i].arr);
            i++;
        }

        free(rows);
        free(columns);

        // Terminate worker nodes
        int i = 1;
        while (i < size) {
            int offset = -1; // Termination signal
            MPI_Send(&offset, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
            i++;
        }
    } else {
        // Worker nodes execute this function
        workerNode(matrices, size, rank);
    }

    MPI_Finalize();

    gettimeofday(&end, 0);
    double elapsed = ((end.tv_sec - begin.tv_sec) * 1000.0) + ((end.tv_usec - begin.tv_usec) / 1000.0);
    if (rank == root) {
        printf("Total execution time: %f milliseconds\n", elapsed);
    }

    return 0;
}
