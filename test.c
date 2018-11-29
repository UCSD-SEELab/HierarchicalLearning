#include <stdio.h>
#include <stdlib.h>

float matrix1[8][10] = {{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
						{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
						{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
						{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
						{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
						{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
						{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
						{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}};
float matrix2[10][12] = {{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
						 {2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0},
						 {3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0},
						 {4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0},
						 {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0},
						 {6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0},
					 	 {7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0},
						 {8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0},
						 {9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0},
						 {10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0}};

float result[8][12] = {0};
						   
/*
 * matrix_multiply: m1*m2=res.
 */
void matrix_multiply(float *m1, int row_m1, int col_m1, 
	float *m2, int row_m2, int col_m2, 
	float *res, int row_res, int col_res) {
	// check whether the number of rows and cols meet the requirement
	if (col_m1 != row_m2 || row_m1 != row_res || col_m2 != col_res)
		return;
	for (int i = 0; i < row_res; ++i) {
		// record the start pointer of i_th line in m1 and res
		float *line_m1 = m1 + i * col_m1;
		float *line_res = res + i * col_res;
		/*for (int j = 0; j < col_res; ++j) {
			float tmp = 0, *tmp_m2 = m2 + j;
			for (int k = 0; k < col_m1; k++) {
				printf("%d %d %d %f %p %f\r\n", i, j, k, *line_m1, line_m1, *tmp_m2);
				tmp += *(line_m1 + k) * *tmp_m2;
				tmp_m2 += col_m2;
			}
			*(line_res++) = tmp;
		}*/
		// walk through each line of m2
		for (int k = 0; k < row_m2; ++k) {
			float tmp_m1 = *(line_m1 + k);
			float *line_m2 = m2 + k * col_m2;
			float tmp = 0;
			// multiply each element in line m1 and certain element in m2
			for (int j = 0; j < col_res; ++j) {
				*(line_res + j) += tmp_m1 * *(line_m2 + j);
			}
		}
	}
}


/*
 * print_matrix
 */
void print_matrix(float *m, int row, int col) {
	for (int i = 0; i < row; ++i) {
		for (int j = 0; j < col; ++j)
			printf("%f ", *(m++));
		printf("\r\n");
	}
	printf("\r\n");
}

int main() {
	matrix_multiply((float *)matrix1, 8, 10, (float *)matrix2, 10, 12, (float *)result, 8, 12);
	print_matrix((float *)result, 8, 12);
	return 0;
}