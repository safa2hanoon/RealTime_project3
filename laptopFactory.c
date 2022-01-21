#include "local.h"

extern int errno;
int shmId[TOTAL_LINES];

int cartonBox[TOTAL_LINES] = {0};
int totalCartons = 0;
int storage_room_condition = 1;
int truck_status = TRUCK_WAITING;
int showRoomCount = 0;
int soldCount = 0;
int lineStatus[TOTAL_LINES] = {LINE_STATUS_RUNNING};
int profit = 0;
bool endSimulation = false;

int storage_employee_delay = 0;
int storage_area_max = 0;
int storage_area_min = 0;
int truck_max_capacity = 0;
int truck_trip_delay = 0;
int salary_ceo = 0;
int salary_hr = 0;
int salary_tech = 0;
int salary_storage = 0;
int salary_truck = 0;
int salary_load = 0;
int cost_fab = 0;
int price_sell = 0;
int profit_threshold = 0;
int step_min_delay = 0;
int step_max_delay = 0;
int profit_max = 0;

char config_file[100];

struct shmseg {
	int stepCount[TOTAL_STEPS];
	int lineId;
	int productResource[PRODUCT_BUFFER];
};

struct lineData {
	int lineNo;
	int stepNo;
};

pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t line_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t storage_room_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t show_room_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t profit_mutex = PTHREAD_MUTEX_INITIALIZER;

//prototyping
void sigintHandler(int);
void *config_parser(void *);
void get_line_number(int *);
int  get_random_value(int, int);
void *step(void *);
void *manufacturing_lines(void *);
void *print_msg(void *);
void *storage_employee(void *);
void *truck_trip(void *);
void *show_room(void *);
void *hr_manager(void *);
void *ceo(void *);
void *end_simulation(void *);

void main(int argc, char **argv) {
    int p_thread  = 0;
    pthread_t threadid[18];
    struct shmseg *shmp;

	signal(SIGINT, sigintHandler);

	if (argc != 2) {
		fprintf(stderr, "Input argument error !\n");
		exit(-1);
	}
	memset(config_file, '\0', 100);
	memcpy(config_file, argv[1], 100);

	p_thread  = pthread_create(&threadid[0], NULL, config_parser, NULL); /* Read from config_file user-defined variables */
	usleep(3000000);
    
	for (int i = 0; i < TOTAL_LINES; i++) {
		shmId[i] = shmget(SHM_KEY + i, sizeof(struct shmseg), 0644 | IPC_CREAT); /* Create unique shared memory for each manufacturing line */
		shmp = shmat(shmId[i], NULL, 0);
		if (shmp == (void *)-1) {
			fprintf(stderr, "Shared memory attach");
		}
		shmp->lineId = i;
		// Initialize the values
		for (int j = 0; j < TOTAL_STEPS; j++) {
			shmp->stepCount[j] = 0;
			shmp->productResource[j] = 0;
			cartonBox[j] = 0;
		}
		totalCartons = 0;
		if (shmdt(shmp) == -1) {
			fprintf(stderr, "shmdt\n");
		}
	}

	for (int i = 1; i < 11; i++) {
		p_thread = pthread_create(&threadid[i], NULL, manufacturing_lines, NULL);
		if (p_thread < 0) {
			fprintf(stderr, "Thread creation error !!!\n");
			exit(-1);
		}
		usleep(500000);
	}
	p_thread  = pthread_create(&threadid[11], NULL, print_msg, NULL);
	p_thread  = pthread_create(&threadid[12], NULL, storage_employee, NULL);
	p_thread  = pthread_create(&threadid[13], NULL, truck_trip, NULL);
	p_thread  = pthread_create(&threadid[14], NULL, show_room, NULL);
	p_thread  = pthread_create(&threadid[15], NULL, hr_manager, NULL);
	p_thread  = pthread_create(&threadid[16], NULL, ceo, NULL);
	p_thread  = pthread_create(&threadid[17], NULL, end_simulation, NULL);
	
	for (int i = 0; i < 18; i++) {
		pthread_join(threadid[i], NULL);
	}

	while (1) {
		usleep(1000000);
	}
}

/*
  Function name : sigintHandler
  Args : integer
  Returns : None

  Description : SIGNINT signal is sent to the program when ctrl + c is pressed in the terminal. The signal 
  ifnot captured will terminate the program at once without clearing the shared memory properly.The signal 
  is captured and the shared memory is cleared before exiting the program.
*/
void sigintHandler(int sig) {
	for (int i = 0; i < TOTAL_LINES; i++) {
		shmctl(shmId[i], IPC_RMID, NULL);
	}
	exit(0);
}

/*
  Function name : config_parser
  Args : file name as pointer
  Returns : None

  Description : This is a thread routine to read the config file and update the corresponding parameters.The
  thread reads the file for every 5 seconds and updated the parameter. Hence, even if the value is changed in 
  the file during runtime it gets updated.
*/
void *config_parser(void *data) {
	FILE *fp;
	char f_line[100];
	int i;
	while (true) {
		i = 0;
		memset(f_line, '\0', 100);
		fp = fopen(config_file, "r");

		if (fp == NULL) {
			fprintf(stderr, "Unable to open file\n");
			exit(1);
		}

		while (fgets(f_line, sizeof(f_line), fp) != NULL) {
			if (i == 0)
				storage_employee_delay = atoi(&f_line[strlen("STORAGE_EMPLOYEE_DELAY=")]);
			else if (i == 1)
				storage_area_max = atoi(&f_line[strlen("STORAGE_AREA_MAX=")]);
			else if (i == 2)
				storage_area_min = atoi(&f_line[strlen("STORAGE_AREA_MIN=")]);
			else if (i == 3)
				truck_max_capacity = atoi(&f_line[strlen("TRUCK_MAX_CAPACITY=")]);
			else if (i == 4)
				truck_trip_delay = atoi(&f_line[strlen("TRUCK_TRIP_DELAY=")]);
			else if (i == 5)
				salary_ceo = atoi(&f_line[strlen("SALARY_CEO=")]);
			else if (i == 6)
				salary_hr = atoi(&f_line[strlen("SALARY_HR=")]);
			else if (i == 7)
				salary_tech = atoi(&f_line[strlen("SALARY_TECH=")]);
			else if (i == 8)
				salary_storage = atoi(&f_line[strlen("SALARY_STORAGE=")]);
			else if (i == 9)
				salary_truck = atoi(&f_line[strlen("SALARY_TRUCK=")]);
			else if (i == 10)
				salary_load = atoi(&f_line[strlen("SALARY_LOAD=")]);
			else if (i == 11)
				cost_fab = atoi(&f_line[strlen("COST_FAB=")]);
			else if (i == 12)
				price_sell = atoi(&f_line[strlen("PRICE_SELL=")]);
			else if (i == 13)
				profit_threshold = atoi(&f_line[strlen("PROFIT_THRESHOLD=")]);
			else if (i == 14)
				step_min_delay = atoi(&f_line[strlen("STEP_MIN=")]);
			else if (i == 15)
				step_max_delay = atoi(&f_line[strlen("STEP_MAX=")]);
			else if (i == 16)
				profit_max = atoi(&f_line[strlen("PROFIT_MAX=")]);

			memset(f_line, '\0', 100);
			i++;
		}
		fclose(fp);
		usleep(5000000);
	}
}

/*
  Function name : get_line_number
  Args : A pointer
  Returns : None

  Description : There are 10 lines in the manufacturing unit. This function generates unique line number 
  between 0 and 9 for each lines. This line number is used to identify the lines.
*/
void get_line_number(int *lno) {
	pthread_mutex_lock(&line_mutex);
	static int line = 0;
	*lno = line;
	line++;
	pthread_mutex_unlock(&line_mutex);
}

/*
  Function name : get_random_value
  Args : int, int
  Return value : int

  Description : Function to generate a random integer between a lower and upper value. This function is used
  to introduce delay during manufacturing steps.
*/
int get_random_value(int lower, int upper) {
	srand(time(0));
	return (rand() % (upper - lower + 1)) + lower;
}

/*
  Function name : step
  Args : struct lineData
  Returns : None

  Description : A thread routine that simulates the work of tech employee in each steps of a
  manufacturing line.

  The step threads are differentiated using step number and line number data.They are passed
  as arguments to the thread.

  step - 1 has a separate logic and step 2 to 5 has a separate logic and step 6 to 10 has a 
  separate logic. They are explained detaily.
*/
void *step(void *data) {
	struct lineData *lData = (struct lineData *)data;
	int shmid, lineNo, stepNo, delay;
	struct shmseg *shmp;
	
	// Gets data from input structure.
	lineNo = lData->lineNo;
	stepNo = lData->stepNo;
	shmid = shmId[lineNo]; // Get the shared memory ID of the corresponding manufacturing line.
	while (1) {
		// Check the storage room and line status.
		if (storage_room_condition && lineStatus[lineNo] == LINE_STATUS_RUNNING) {
			delay = get_random_value(step_min_delay, step_max_delay);
			shmp = shmat(shmid, NULL, 0); // Attach to the shared memory.
			if (shmp == (void *)-1) {
				fprintf(stderr, "Shared memory attach");
			}
			if (stepNo == 0) { // This logic is only for step 0 (step 1 in real time).
				for (int i = 0; i < PRODUCT_BUFFER; i++) {// Iterates all elements in product buffer. This is similar to buffer of laptop spare parts.
					if (shmp->productResource[i] & (1 << stepNo)) { // Checks if 0th bit is set.
					}
					else { // If the bit is not set
						usleep(delay * 1000000); // Take time to perform step 0
						shmp->productResource[i] |= 1 << stepNo; // Set the 0th bit
						shmp->stepCount[stepNo] += 1; // Increase the count in corresponding step number.
						break;
					}
				}
			}
			else if (stepNo > 0 && stepNo < 5) { // Logic for step 1 to 4 (step 2 to 5 in real time)
				for (int i = 0; i < PRODUCT_BUFFER; i++) { // Iterates all elements in product buffer. This is similar to buffer of laptop spare parts.
		            // Check if previous step is completed. Example if it is step 4 check step 3 is completed by checking the corresponding bit.
					if (shmp->productResource[i] & (1 << (stepNo - 1))) {
						if (shmp->productResource[i] & (1 << stepNo)) {
						}
						else {
							usleep(delay * 1000000);				 // Take time to perform the step.
							shmp->productResource[i] |= 1 << stepNo; // Set the stepNo bit.
							shmp->stepCount[stepNo] += 1;
							shmp->stepCount[stepNo - 1] -= 1;
							break;
						}
					}
				}
			}
			else {
				for (int i = 0; i < PRODUCT_BUFFER; i++) { // Iterates all elements in product buffer. This is similar to buffer of laptop spare parts.
					if (shmp->productResource[i] >= 31) { // Checks if step 1 to step 5 are completed by checking first 5 bits. If first 5 bits are set value will be 31 or more.
						if (shmp->productResource[i] & (1 << stepNo)) {
						}
						else {
							usleep(delay * 1000000); // Take time to perform the step.
							shmp->productResource[i] |= 1 << stepNo; // Set the stepNo bit.
							shmp->stepCount[stepNo] += 1;
							break;
						}
					}
				}
				// Employee at step 10 has additional task of filling up the cartons.
				if (stepNo == 9) {
					for (int i = 0; i < PRODUCT_BUFFER; i++) { // Iterates all elements in product buffer. This is similar to buffer of laptop spare parts.
						if (shmp->productResource[i] == 1023) { // If all 10 steps are done then 10 bits will be set which is equal to 1023.
							pthread_mutex_lock(&storage_room_mutex);
							cartonBox[lineNo]++; // Increase the laptops in carton box.
							pthread_mutex_unlock(&storage_room_mutex);
							for (int k = 4 ; k < 10; k++) {
								shmp->stepCount[k] -= 1; // Reduce counts for all steps from 5 to 10.
							}
							shmp->productResource[i] = 0; // Clear all bits.
						}
					}
				}
			}
			if (shmdt(shmp) == -1) { // Detach the shared memory.
				fprintf(stderr, "shmdt\n");
			}
		}
		usleep(1000000);
	}
}

/*
  Function name : manufacturing_lines
  Args : None
  Returns : None

  Description : A thread routine that simulates the manufacturing lines. Each line creates 10 threads 
  representing the steps from 1 to 10.
*/
void *manufacturing_lines(void *data) {
	int p_thread = 0;
	pthread_t threadid[10];
	struct lineData lData;
	int line;

	get_line_number(&line); // Get unique line number.
	lData.lineNo = line;	 // Send line number as argument to step thread.

	for (int i = 0; i < TOTAL_STEPS; i++) {
		lData.stepNo = i; // Send step number as argument to step thread.
		p_thread = pthread_create(&threadid[i], NULL, step, &lData);
		if (p_thread < 0) {
			fprintf(stderr, "Thread creation error !\n");
			exit(-1);
		}
		usleep(500000);
	}

	for (int i = 0; i < TOTAL_STEPS; i++) {
		pthread_join(threadid[i], NULL);
	}

	while (1) {
		usleep(500000);
	}
}

/*
  Function name : print_msg
  Args : None
  Returns : None

  Description : It is a thread routine that prints the information to the user.
*/
void *print_msg(void *data) {
	struct shmseg *shmp;

	while (true) {
		system("clear");
		pthread_mutex_lock(&console_mutex);
		fprintf(stdout, "\n\n\tManufacturing Line No\tStep - 1\tStep - 2\tStep - 3\tStep - 4\tStep - 5\tStep - 6\tStep - 7\tStep - 8\tStep - 9\tStep -10 LaptopsInCartonBox\n");
		for (int i = 0; i < TOTAL_LINES; i++) {
			shmp = shmat(shmId[i], NULL, 0);
			if (shmp == (void *)-1) {
				fprintf(stderr, "Shared memory attach");
			}
			fprintf(stdout, "\t%d", shmp->lineId + 1);
			if (lineStatus[shmp->lineId] == LINE_STATUS_RUNNING) {
				fprintf(stdout, " - Running");
			}
			else {
				fprintf(stdout, "- Suspended");
			}

			for (int j = 0; j < TOTAL_STEPS; j++) {
				fprintf(stdout, "\t\t  %d", shmp->stepCount[j]);
			}
			pthread_mutex_lock(&storage_room_mutex);
			fprintf(stdout, "\t\t  %d", cartonBox[i]);
			pthread_mutex_unlock(&storage_room_mutex);
			fprintf(stdout, "\n");

			if (shmdt(shmp) == -1) {
				fprintf(stderr, "shmdt\n");
			}
		}
		pthread_mutex_lock(&storage_room_mutex);
		fprintf(stdout, "\nTotal number of Cartons in storage room : %d\n", totalCartons);
		pthread_mutex_unlock(&storage_room_mutex);

		if (!storage_room_condition)
			fprintf(stdout, "\nPRODUCTION STATUS : STOPPED !!! STORAGE ROOM MAX CAPACITY REACHED !!!\n");
		else
			fprintf(stdout, "\nPRODUCTION STATUS : RUNNING !!!\n");

		if (truck_status == TRUCK_WAITING) {
			fprintf(stdout, "\nTRUCK STATUS : WAITING !!! STORAGE ROOM EMPTY !!!\n");
		}
		else if (truck_status == TRUCK_LOADING) {
			fprintf(stdout, "\nTRUCK STATUS : LOADING GOODS!!!\n");
		}
		else {
			fprintf(stdout, "\nTRUCK STATUS : ON TRIP!!!\n");
		}

		pthread_mutex_lock(&show_room_mutex);
		fprintf(stdout, "\nLAPTOPS IN SHOWROOM : %d\n", showRoomCount);
		fprintf(stdout, "\nLAPTOPS SOLD : %d\n", soldCount);
		pthread_mutex_unlock(&show_room_mutex);
		pthread_mutex_lock(&profit_mutex);
		fprintf(stdout, "\nTOTAL PROFIT : %d\n", profit);
		pthread_mutex_unlock(&profit_mutex);

		pthread_mutex_unlock(&console_mutex);
		if (endSimulation) {
			fprintf(stdout, "\n\t\t\t SIMULATION ENDS \n");
			for (int i = 0; i < TOTAL_LINES; i++) {
				shmctl(shmId[i], IPC_RMID, NULL);
			}
			exit(0);
		}
		usleep(3000000);
	}
}

/*
  Function name : storage_employee
  Args : None
  Returns : None

  Description : A thread routine that represents the job of a storage worker.
 */
void *storage_employee(void *data) {
	int max = 0;
	int maxIndex = 0;

	while (true) {
		// storage_room_condition - used to identify if the room is full.
		if (storage_room_condition) {
			pthread_mutex_lock(&storage_room_mutex);
			// Loop that finds which manufacturing line has produce more laptops.
			for (int i = 0; i < TOTAL_LINES; i++) {
				if (cartonBox[i] > max) {
					max = cartonBox[i];
					maxIndex = i;
				}
			}
			pthread_mutex_unlock(&storage_room_mutex);

			// Compares the max value with number of laptops per cartons.
			if (max >= LAPTOPS_PER_CARTONS) {
				pthread_mutex_lock(&storage_room_mutex);
				// The worker takes the laptops from cartons.
				cartonBox[maxIndex] -= LAPTOPS_PER_CARTONS;
				pthread_mutex_unlock(&storage_room_mutex);

				// Time taken by the storage employee to arrange the cartons in storage room.
				usleep(storage_employee_delay * 1000000);

				pthread_mutex_lock(&storage_room_mutex);
				// Increase the number of cartons in the storage room.
				totalCartons++;
				if (totalCartons >= storage_area_max) {
					// Checks if the storage room has reached max number of cartons.
					// If yes, set the variable to 0. Based on this the production line will be paused.
					storage_room_condition = 0;
				}
				pthread_mutex_unlock(&storage_room_mutex);
				max = 0;
				maxIndex = 0;
			}
		}
		usleep(500000);
	}
}

/*
  Function name : truck_trip
  Args : None
  Returns : None

  Description : A thread routine that simulates a truck.The routine checks the storage room and loads the truck upto the
  maximum value. It takes some time to reach the show room and unload the truck.
 */
void *truck_trip(void *data) {
	int cartonsInTruck = 0;

	while (true) {
		pthread_mutex_lock(&storage_room_mutex);
		// The truck is in waiting state when storage room is empty.
		if (totalCartons == 0 && cartonsInTruck == 0) {
			truck_status = TRUCK_WAITING;
		}
		// Checks if the truck has reached maximum capacity.
		if (totalCartons > 0 && cartonsInTruck <= truck_max_capacity) {
			// load the truck.
			cartonsInTruck++;
			// Reduce the carton in storage area.
			totalCartons--;
			if (totalCartons < storage_area_max)
				storage_room_condition = 1; // set the storage room condition to 1
			truck_status = TRUCK_LOADING;	// Truk is in loading state.
		}
		pthread_mutex_unlock(&storage_room_mutex);

		if (cartonsInTruck == truck_max_capacity) {	 // Checks if truck has reached maximum capacity.
			truck_status = TRUCK_ON_TRIP; // Truck is trip state
			usleep(truck_trip_delay * 1000000);
			pthread_mutex_lock(&show_room_mutex);
			showRoomCount = cartonsInTruck * LAPTOPS_PER_CARTONS; // Increases the laptop number in showroom.
			pthread_mutex_unlock(&show_room_mutex);	// Time taken by the truck to reach the showroom and unload the laptops.
			cartonsInTruck = 0;
		}
		usleep(500000);
	}
}

/*
  Function name : show_room
  Args : None
  Returns : None

  Description : A thread routine that simulates a show room that sells laptop. For testing purpose, we have set that for every
  5 seconds, define number of laptops are sold.
 */
void *show_room(void *data) {
	while (true) {
		usleep(5000000);
		pthread_mutex_lock(&show_room_mutex);
		if (showRoomCount > SOLD_LAPTOPS) {
			// Laptops sold and update the variables.
			showRoomCount -= SOLD_LAPTOPS;
			soldCount += SOLD_LAPTOPS;
		}
		pthread_mutex_unlock(&show_room_mutex);
	}
}


/*
  Function name : hr_manager
  Args : None
  Returns : None

  Description : A thread routine that simulates the job of a hr.
  It calculates the total expense and total laptops sold and profit percentage.
  Based on the profit percentage the CEO takes decision.

  For testing purpose it is set such that a minimum of laptops has to be sold
  to calculate the profit.
 */
void *hr_manager(void *data) {
	int laptopSellPrice = 0;
	int laptopCostPrice = 0;
	int salary = 0;
	int totalExpense = 0;

	while (true) {
		salary = 0;
		pthread_mutex_lock(&show_room_mutex);
		// Checks if at least minimum number of laptops are sold.
		if (soldCount > MIN_LAPTOPS_PER_PROFIT) {
			// Calculate laptop selling and cost price.
			laptopSellPrice = price_sell * soldCount;
			laptopCostPrice = cost_fab * showRoomCount;
			for (int i = 0; i < TOTAL_LINES; i++) {
				// Checks if the manufacturing line is running. Technical employees in the
				// running manufacturing line are only given salary.
				if (lineStatus[i] == LINE_STATUS_RUNNING)
					salary += 10 * salary_tech; // Each line contains 10 tech employees.
			}
			salary += salary_ceo + salary_hr + salary_storage + salary_truck + salary_load; // Calculates total salary.
			totalExpense = salary + laptopCostPrice;
			pthread_mutex_lock(&profit_mutex);
			profit = (laptopSellPrice - totalExpense) / 100; // calculates profit.
			pthread_mutex_unlock(&profit_mutex);
		}
		pthread_mutex_unlock(&show_room_mutex);
		usleep(1000000);
	}
}

/*
  Function name : ceo
  Args : None
  Returns : None

  Description : A thread routine that simulates the job of a CEO. The CEO compares the profit with the profit threshold
  defined by the user. If the profit goes down the threshold value, the CEO suspends one of the manufacturing lines. CEO 
  checks the profit every 10 seconds. Each time if the profit is less than the threshold he suspends a manufacturing line.

  Similary if the profit reaches above the threshold, the CEO then make the suspended manufacturing line active.*/
  
void *ceo(void *data) {
	while (true) {
		pthread_mutex_lock(&show_room_mutex);
		if (soldCount > MIN_LAPTOPS_PER_PROFIT) {
			pthread_mutex_lock(&profit_mutex);
			// Checks if the profit is less than the threshold.
			if (profit < profit_threshold) {
				for (int i = 0; i < TOTAL_LINES; i++) {
					// Checks if the manufacturing line is running.
					if (lineStatus[i] == LINE_STATUS_RUNNING) {
						lineStatus[i] = LINE_STATUS_SUSPENDED; // Suspend the manufacturing line.
						break;
					}
				}
			}
			else {
				for (int i = 0; i < TOTAL_LINES; i++) {
					// checks if the manufacturing line is suspended.
					if (lineStatus[i] == LINE_STATUS_SUSPENDED) {
						lineStatus[i] = LINE_STATUS_RUNNING; // Run the manufacturing line.
						break;
					}
				}
			}
			pthread_mutex_unlock(&profit_mutex);
		}
		pthread_mutex_unlock(&show_room_mutex);
		usleep(10000000);
	}
}

/*
  Function name : end_simulation
  Args : None
  Returns : None

  Description : A thread routine that checks the condition to the end the simulation.
  The conditions are, profit reaching maximum threshold value or more than 50% employees
  suspended.
*/
void *end_simulation(void *data) {
	int count;
	while (true) {
		count = 0;
		pthread_mutex_lock(&profit_mutex);
		if (profit >= profit_max) // Checks if profit is more than the threshold.
			endSimulation = true; //
		pthread_mutex_unlock(&profit_mutex);

		for (int i = 0; i < 10; i++) {
			if (lineStatus[i] == LINE_STATUS_SUSPENDED) // checks if the line is running or suspended.
				count++; // counts the number of suspended lines.
		}
		if (count > 5)
			endSimulation = true;
		usleep(3000000);
	}
}
