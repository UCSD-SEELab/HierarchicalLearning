/*
 * Project: Establish MQTT connection with RPi and send data after run linear regression
 * Suppose necessary files are alreay written on the SD card:
 *     data.txt (extracted_subject1.txt)
 * Author: Xiaofan Yu
 * Date: 11/2/2018
 */
#include "application.h"
#include "MQTT.h"

#define MAX_RAND_NUM 1000000 // maximum number in random()
#define MAX_INFO_LENGTH 100 // maximum length of sending info, e.g. bw and sample rate
#define SEND_TIMES 1000 // the times that we do sending experiments
#define FEATURE_NUM 12 // number of features, linear regression input
#define CLASSES_NUM 5 // number of classes, linear regression output
#define BATCH_LEN 200 // the number of input samples for one time

// const variables in experiments
const float sample_data[FEATURE_NUM] = {1.5663333, 2.0231032, 4.368568, -0.7739354,
	0.6374992, 36494.797, 11866.794, -0.57553095, 1583.8704, -0.54552644, 8.296114, 1.6720939};
const unsigned int copy_size = FEATURE_NUM * sizeof(float);

// global variables for MQTT
byte server[] = { 137,110,160,230 }; // specify the ip address of RPi
 // ip, port, keepalive, callback, maxpacketsize
MQTT client(server, 1883, 60, NULL, max(BATCH_LEN, MAX_INFO_LENGTH)+10);

// global array to store float data for linear regression
float lr_para[FEATURE_NUM][CLASSES_NUM]; // parameters for linear regression
float lr_in[BATCH_LEN][FEATURE_NUM], lr_out[BATCH_LEN][CLASSES_NUM];
// glabal array to store send batch
uint8_t batch_data[BATCH_LEN+10]; // send batch
unsigned int len_of_batch = 0; // len of send batch
// global array to store assistant info
char time_measure[MAX_INFO_LENGTH];

/*************************************************************************
 * linear regression related code
 ************************************************************************/
/*
 * init_lr_parameter
 */
void init_lr_parameter() {
	long randNum;
	for (uint8_t i = 0; i < FEATURE_NUM; ++i)
		for (uint8_t j = 0; j < CLASSES_NUM; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			lr_para[i][j] = ((float)(randNum - MAX_RAND_NUM)) / MAX_RAND_NUM;
			// Serial.printf("%f ", para_num[i][j]);
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
		for (int j = 0; j < col_res; ++j) {
			float tmp = 0;
			// record the start pointer of j_th column in m2, save energy
			float *column_m2 = m2 + j;
			for (int k = 0; k < col_m1; ++k) {
				// tmp += *(line_m1 + k) * *(column_m2 + k * col_m2);
				tmp += *(line_m1++) * *column_m2;
				column_m2 += col_m2;
			}
			*(line_res++) = tmp; // set the value
		}
	}
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
/*************************************************************************
 * Other settings for local connector
 ************************************************************************/
/*
 * read_matrix - fit the sample data into the matrix
 */
void read_matrix(float *in, int in_row, int in_col) {
	if (in_col != FEATURE_NUM) // check whether in_col matches with feature num
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
	init_lr_parameter(); // init lr parameters

	// variables used in time testing
	unsigned long comp_start, total_comp_time; // to record the total time for calculating avg
	float comp_avg; // to record the average time of computation

	// connect to the RPi
	// client_id, user, passwd, willTopic, willQoS, willRetain, willMessage, cleanSession, version?
	client.connect("photon", "xiaofan", "0808", 0, client.QOS2, 0, 0, true, client.MQTT_V311);
	Serial.println("Connect!");

	total_comp_time = 0;
	// run the experiments for SEND_TIMES times
	for (int send_num = 0; send_num < SEND_TIMES; ++send_num) {
		len_of_batch = 0;
		// read sample_rate lines
		read_matrix((float *)lr_in, BATCH_LEN, FEATURE_NUM);
		// do local computation
		comp_start = millis();
		// compute lr for this input matrix
		matrix_multiply((float *)lr_in, BATCH_LEN, FEATURE_NUM, 
			(float *)lr_para, FEATURE_NUM, CLASSES_NUM,
			(float *)lr_out, BATCH_LEN, CLASSES_NUM);
		// pack the result into send batch
		pack((float *)lr_out, BATCH_LEN, CLASSES_NUM);
		// publish computation result
		if (client.isConnected()) {
			client.publish("data", batch_data, len_of_batch, false, client.QOS2, false, NULL);
			while (!client.loop_QoS2()); // block and wait for pub done
		}
		Serial.printf("publish %d bytes successfully!\r\n", len_of_batch);
		// update total time
		total_comp_time += millis() - comp_start; // cumulative add
		Serial.println(send_num);
	}
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
