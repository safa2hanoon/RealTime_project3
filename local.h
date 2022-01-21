#ifndef __LOCAL_H_
#define __LOCAL_H_

/*
 * Common header file
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define SHM_KEY 0x3000 
#define TOTAL_STEPS 10
#define TOTAL_LINES 10
#define PRODUCT_BUFFER 10
#define LAPTOPS_PER_CARTONS 10
#define TRUCK_WAITING 0
#define TRUCK_LOADING 1
#define TRUCK_ON_TRIP 2
#define LINE_STATUS_RUNNING 0
#define LINE_STATUS_SUSPENDED 1
#define SOLD_LAPTOPS 5
#define MIN_LAPTOPS_PER_PROFIT 10
#endif
