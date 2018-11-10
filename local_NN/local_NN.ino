/*
 * Project: Establish MQTT connection with RPi and send data after running
 *     basic neural network. Use fixed sample data.
 * Author: Xiaofan Yu
 * Date: 11/7/2018
 */
#include "application.h"
#include "MQTT.h"

#define MAX_RAND_NUM 1000000 // maximum number in random()
#define MAX_INFO_LENGTH 100 // maximum length of sending info, e.g. time consumption
#define SEND_TIMES 1000 // the times that we do sending experiments
#define FEATURE_NUM 12 // number of features, linear regression input
#define CLASSES_NUM 5 // number of classes, linear regression output
// size of neural network
#define N_IN FEATURE_NUM // nodes as input
#define N_OUT CLASSES_NUM // nodes as output
#define N_1 98 // nodes in the first layer
#define N_2 98 // nodes in the second layer
#define BATCH_LEN 1 // the number of input samples for one time

// const variables in experiments
const float sample_data[FEATURE_NUM] = {1.5663333, 2.0231032, 4.368568, -0.7739354,
	0.6374992, 36494.797, 11866.794, -0.57553095, 1583.8704, -0.54552644, 8.296114, 1.6720939};
const unsigned int copy_size = FEATURE_NUM * sizeof(float);

// global variables for MQTT
byte server[] = { 137,110,160,230 }; // specify the ip address of RPi
// ip, port, keepalive, callback, maxpacketsize
MQTT client(server, 1883, 60, NULL, max(SEND_TIMES*BATCH_LEN, MAX_INFO_LENGTH)+10);

// nodes for neural networks
float nn_in[BATCH_LEN][N_IN], nn_out[BATCH_LEN][N_OUT]; // input and output nodes
float nn_l1[BATCH_LEN][N_1], nn_l2[BATCH_LEN][N_2]; // middle layer nodes in nn
float nn_w1[N_IN][N_1], nn_w2[N_1][N_2], nn_w3[N_2][N_OUT]; // weights between layers
// glabal array to store send batch
uint8_t batch_data[SEND_TIMES*BATCH_LEN+10]; // send batch
unsigned int len_of_batch = 0; // len of send batch
// global array to store time measurements
char time_measure[MAX_INFO_LENGTH];

/*************************************************************************
 * NN functions
 ************************************************************************/
/*
 * init_weights
 */
void init_nn_weights() {
	long randNum;
	for (int i = 0; i < N_IN; ++i)
		for (int j = 0; j < N_1; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			nn_w1[i][j] = ((float)randNum) / MAX_RAND_NUM;
		}
	for (int i = 0; i < N_1; ++i)
		for (int j = 0; j < N_2; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			nn_w2[i][j] = ((float)randNum) / MAX_RAND_NUM;
		}
	for (int i = 0; i < N_2; ++i)
		for (int j = 0; j < N_OUT; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			nn_w3[i][j] = ((float)randNum) / MAX_RAND_NUM;
		}
}

/*
 * ReLU activation. Read continuously!
 */
inline void ReLU(float *x, int row, int col) {
	for (int i = 0; i < row; ++i) {
		float *line = x + i * col; // record to save energy
		for (int j = 0; j < col; ++j)
			*(line + j) > 0? 1: *(line + j) = 0; // do nothing or make it zero
	}
}

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
		// walk through each line of m2
		for (int k = 0; k < col_m1; ++k) {
			float *line_m2 = m2 + k * col_m2;
			float tmp = 0, tmp_m2 = *line_m2;
			// multiply each element in line m1 and certain element in m2
			for (int j = 0; j < col_res; ++j) {
				tmp += *(line_m1++) * tmp_m2;
			}
			*(line_res++) += tmp; // add this to each element in res
			line_m2++;
		}
	}
}

/*
 * nn: in -> nn -> out
 * "in" is feature_num * batch_num, "out" is classnum*batch_num
 */
void nn(float *in, int in_row, int in_col,
	    float *out, int out_row, int out_col) {
	// check whether the rows of input and output equal
	if (in_row != out_row || in_col != N_IN || out_col != N_OUT)
		return;
	matrix_multiply(in, in_row, in_col,
					(float *)nn_w1, N_IN, N_1,
					(float *)nn_l1, in_row, N_1);
	ReLU((float *)nn_l1, in_row, N_1);

	matrix_multiply((float *)nn_l1, in_row, N_1,
					(float *)nn_w2, N_1, N_2,
					(float *)nn_l2, in_row, N_2);
	ReLU((float *)nn_l2, in_row, N_2);

	matrix_multiply((float *)nn_l2, in_row, N_2,
					(float *)nn_w3, N_2, N_OUT,
					out, out_row, out_col);
	ReLU(out, out_row, out_col);
}

/*
 * pack: pack out to batch_data. 
 * out_col should equal to N_OUT as we read continuously!
 */
void pack(float *out, int out_row, int out_col) {
	for (int i = 0; i < out_row; ++i) {
		float max = 0;
		uint8_t max_index = 0;
		float *line = out + i * out_col; // record to save energy
		// go through the out array and find out argmax-index
		for (int j = 0; j < out_col; ++j) {
			if (*line > max) {
				max = *(line++);
				max_index = j;
			}
		}
		batch_data[len_of_batch++] = max_index;
		// Serial.printf("max index is %d (%d)\r\n", max_index, len_of_batch);
	}
}

/*
 * print_matrix
 */
/*inline void print_matrix(float *m, int row, int col) {
	for (int i = 0; i < row; ++i) {
		for (int j = 0; j < col; ++j)
			Serial.printf("%f ", *(m+i*col+j));
		Serial.printf("\r\n");
	}
	Serial.printf("\r\n");
}*/

/*************************************************************************
 * Other settings for local connector
 ************************************************************************/
/*
 * read_matrix - fit the sample data into the matrix
 */
void read_matrix(float *in, int in_row, int in_col) {
	if (in_col != N_IN) // check whether in_col matches with feature num
		return;
	for (int i = 0; i < in_row; ++i) {
		memcpy(in, sample_data, copy_size);
		in += in_col;
	}
}

/*
 * setup
 */
void setup() {
	Serial.begin(115200);
	init_nn_weights(); // init nn weights

	// variables used in time testing
	unsigned long comp_start, total_comp_time; // to record the total time for calculating avg
	float comp_avg; // to record the average time of computation

	// connect to the RPi
	// client_id, user, passwd, willTopic, willQoS, willRetain, willMessage, cleanSession, version?
	client.connect("photon", "xiaofan", "0808", 0, client.QOS2, 0, 0, true, client.MQTT_V311);
	Serial.println("Connect!");

	total_comp_time = len_of_batch = 0;
	// run the experiments for SEND_TIMES times
	for (int send_num = 0; send_num < SEND_TIMES; ++send_num) {
		// read sample_rate lines
		read_matrix((float *)nn_in, BATCH_LEN, N_IN);
		// do local computation
		comp_start = millis();
		// compute nn for this input matrix
		nn((float *)nn_in, BATCH_LEN, N_IN, (float *)nn_out, BATCH_LEN, N_OUT);
		// pack the result into send batch
		pack((float *)nn_out, BATCH_LEN, N_OUT);
		// update total time
		total_comp_time += millis() - comp_start; // cumulative add
		Serial.println(send_num);
	}
	// publish computation result
	if (client.isConnected()) {
		client.publish("data", batch_data, len_of_batch, false, client.QOS2, false, NULL);
		while (!client.loop_QoS2()); // block and wait for pub done
	}
	Serial.printf("publish %d bytes successfully!\r\n", len_of_batch);

	// compute the avg time consumption and send it
	comp_avg = (float)total_comp_time / SEND_TIMES;
	sprintf(time_measure, "comp avg:%f\r\n", comp_avg);
	Serial.print(time_measure);
	// publish time test result
	if (client.isConnected()) {
		client.publish("time", (uint8_t *)time_measure, strlen(time_measure), 
			false, client.QOS2, false, NULL);
		while (!client.loop_QoS2()); // block and wait for pub done
	}
	Serial.printf("publish time measurments successfully!\r\n");
	client.disconnect();
}

/*
 * loop
 */
void loop() {
}
