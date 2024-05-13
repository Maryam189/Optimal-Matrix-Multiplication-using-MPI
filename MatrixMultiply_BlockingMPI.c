#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define MAX_STACK_SIZE 10

typedef struct {
    double** matrix;
    int rows, cols;
} Matrix;

Matrix matrixStack[MAX_STACK_SIZE];
int stackIndex = -1;

void pushMatrixStack(Matrix m) {
    if (stackIndex < MAX_STACK_SIZE - 1) {
        matrixStack[++stackIndex] = m;
    }
}

void popMatrixStack() {
    if (stackIndex > -1) {
        stackIndex--;
    }
}

Matrix topMatrixStack() {
    return stackIndex > -1 ? matrixStack[stackIndex] : (Matrix) { .rows = 0, .cols = 0, .matrix = NULL };
}

void loadMatrixDimensions(const char* filename, int** rows, int** cols, int* matrixCount) {
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

    *rows = (int*)malloc(*matrixCount * sizeof(int));
    *cols = (int*)malloc(*matrixCount * sizeof(int));

    int i = 0;
    while (i < *matrixCount) {
        fscanf(file, "%d %d", &(*rows)[i], &(*cols)[i]);
        i++;
    }

    fclose(file);
}

void generateRandomMatrices(Matrix* matrices, int matrixCount, int* rows, int* cols) {
    srand(time(0));

    int i = 0;
    while (i < matrixCount) {
        matrices[i].rows = rows[i];
        matrices[i].cols = cols[i];

        matrices[i].matrix = (double**)malloc(rows[i] * sizeof(double*));

        int a = 0;
        while (a < rows[i]) {
            matrices[i].matrix[a] = (double*)malloc(cols[i] * sizeof(double));
            a++;
        }

        int b = 0;
        while (b < matrices[i].rows) {
            int c = 0;
            while (c < matrices[i].cols) {
                matrices[i].matrix[b][c] = rand() % 10;
                c++;
            }
            b++;
        }

        i++;
    }
}

void printMatrix(Matrix matrix) {
    int a = 0;
    while (a < matrix.rows) {
        int b = 0;
        while (b < matrix.cols) {
            printf("%f  ", matrix.matrix[a][b]);
            b++;
        }
        printf("\n");
        a++;
    }
    printf("\n");
}

void workerNode(Matrix* matrices, int nodeSize, int nodeRank) {
    MPI_Status nodeStatus;

    while (1) {
        int source = 0;
        int offset = 0, row1 = 0, col1 = 0, row2 = 0, col2 = 0;

        MPI_Recv(&offset, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &nodeStatus);
        if (offset == -1) {
            // Termination signal received
            break;
        }

        MPI_Recv(&row1, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &nodeStatus);
        MPI_Recv(&col1, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &nodeStatus);
        MPI_Recv(&row2, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &nodeStatus);
        MPI_Recv(&col2, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &nodeStatus);

        double resultMat[row1][col2];
        double matA[row1][col1];
        double matB[row2][col2];

        MPI_Recv(&matA, row1 * col1, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &nodeStatus);
        MPI_Recv(&matB, row2 * col2, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &nodeStatus);

        // Perform matrix multiplication
        int k = 0;
        while (k < col2) {
            int i = 0;
            while (i < row1) {
                resultMat[i][k] = 0.0;
                int j = 0;
                while (j < col1) {
                    resultMat[i][k] += matA[i][j] * matB[j][k];
                    j++;
                }
                i++;
            }
            k++;
        }

        // Send results back to the root
        MPI_Send(&offset, 1, MPI_INT, source, 2, MPI_COMM_WORLD);
        MPI_Send(&row1, 1, MPI_INT, source, 2, MPI_COMM_WORLD);
        MPI_Send(&resultMat, row1 * col2, MPI_DOUBLE, source, 2, MPI_COMM_WORLD);
    }

    // Free allocated memory if needed
    int i = 0;
    while (i < nodeSize) {
        int j = 0;
        while (j < matrices[i].rows) {
            free(matrices[i].matrix[j]);
            j++;
        }
        free(matrices[i].matrix);
        i++;
    }
    free(matrices);
}

int main(int argc, char* argv[]) {
    struct timeval beginTime, endTime;
    gettimeofday(&beginTime, 0);

    int matrixCount = 0;
    int* rows;
    int* cols;
    int rootNode = 0;

    MPI_Status mpiStatus;

    int nodeRank, nodeSize;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nodeSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &nodeRank);
    Matrix matrices[matrixCount];
    if (nodeRank == rootNode) {
        char filename[] = "dimensions.txt";
        loadMatrixDimensions(filename, &rows, &cols, &matrixCount);
        Matrix matrices[matrixCount];
        generateRandomMatrices(matrices, matrixCount, rows, cols);

        int i = 0;
        while (i < matrixCount) {
            printf("Matrix : %d \n", i + 1);
            printMatrix(matrices[i]);
            i++;
        }

        // Perform matrix multiplication
        char multiplicationOrder[50];
        int multiplicationCost;
        FILE* file = fopen("cost.txt", "r");

        if (file == NULL) {
            printf("Error opening the file.\n");
            return 1;
        }

        int readResult = fscanf(file, "%49s %d", multiplicationOrder, &multiplicationCost);
        if (readResult == 2) {
            printf("Read from file: Order = %s, Cost = %d\n", multiplicationOrder, multiplicationCost);
        } else {
            printf("Failed to read from file.\n");
        }

        fclose(file);
        printf("Order: %s \n", multiplicationOrder);

        // Perform matrix multiplication based on the order
        double** finalResultMatrix = matrices[multiplicationOrder[0] - 'A'].matrix;

        int i = 1;
        while (i < matrixCount) {
            int currentMatrixIndex = multiplicationOrder[i] - 'A';
            int previousMatrixIndex = multiplicationOrder[i - 1] - 'A';

            // Perform matrix multiplication
            double** currentMatrix = matrices[currentMatrixIndex].matrix;
            double** previousResultMatrix = finalResultMatrix;

            int row1 = matrices[previousMatrixIndex].rows;
            int col1 = matrices[previousMatrixIndex].cols;
            int row2 = matrices[currentMatrixIndex].rows;
            int col2 = matrices[currentMatrixIndex].cols;

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
            finalResultMatrix = tempResult;
            i++;
        }

        // Display the final result matrix
        printf("Result Matrix \n");
        int a = 0;
        while (a < matrices[multiplicationOrder[matrixCount - 1] - 'A'].rows) {
            int b = 0;
            while (b < matrices[multiplicationOrder[matrixCount - 1] - 'A'].cols) {
                printf("%f  ", finalResultMatrix[a][b]);
                b++;
            }
            printf("\n");
            a++;
        }

        // Free allocated memory
        int i = 0;
        while (i < matrixCount) {
            int j = 0;
            while (j < matrices[i].rows) {
                free(matrices[i].matrix[j]);
                j++;
            }
            free(matrices[i].matrix);
            i++;
        }

        free(rows);
        free(cols);

        // Terminate worker nodes
        int i = 1;
        while (i < nodeSize) {
            int offset = -1; // Termination signal
            MPI_Send(&offset, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
            i++;
        }
    } else {
        // Worker nodes execute this function
        workerNode(matrices, nodeSize, nodeRank);
    }

    MPI_Finalize();

    gettimeofday(&endTime, 0);
    double elapsedTime = ((endTime.tv_sec - beginTime.tv_sec) * 1000.0) + ((endTime.tv_usec - beginTime.tv_usec) / 1000.0);
    if (nodeRank == rootNode) {
        printf("Total execution time: %f milliseconds\n", elapsedTime);
    }

    return 0;
}
