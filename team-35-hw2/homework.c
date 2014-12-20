/*
 * file:        homework.c
 * description: Skeleton code for CS 5600 Homework 2
 *
 * Peter Desnoyers, Northeastern CCIS, 2011
 * $Id: homework.c 530 2012-01-31 19:55:02Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include "hw2.h"

/********** YOUR CODE STARTS HERE ******************/

/*
 * Here's how you can initialize global mutex and cond variables
 */
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C = PTHREAD_COND_INITIALIZER;

pthread_cond_t customer_waiting = PTHREAD_COND_INITIALIZER;
pthread_cond_t haircut = PTHREAD_COND_INITIALIZER;
pthread_cond_t barber_seat_occupied = PTHREAD_COND_INITIALIZER;

int remaining_seats = 4;

/*
    These are const variables.
 */
#define CUSTOMER_SLEEPING_TIME 10.0
#define HAIRCUT_TIME 1.2
#define SATISFIED_WITH_HAIR_TIME 1

#define TOTAL_CUSTOMERS 10

/*
    Variables for q3.
 */
int total_visit = 0;
int fullshop_visit = 0;

void *timer;
void *customer_in_shop_counter;
void *customer_in_barber_chair_counter;


/* the barber method
 */
void barber(void)
{
    pthread_mutex_lock(&m);
    while (1)
    {
        /* your code here */
        while (remaining_seats >= 4){
            printf("DEBUG: %f barber goes to sleep\n", timestamp());
            pthread_cond_wait(&customer_waiting, &m);
            printf("DEBUG: %f barber wakes up\n", timestamp());
        }
        // printf("DEBUG: At %f, barber is starting doing haircut.\n", timestamp());
        pthread_cond_signal(&haircut);
        pthread_cond_wait(&barber_seat_occupied, &m);
        // printf("DEBUG: At %f, barber has finished haircut.\n", timestamp());
    }
    pthread_mutex_unlock(&m);
}

/* the customer method
 */
void customer(int customer_num)
{
    pthread_mutex_lock(&m);
    /* your code here */
    total_visit ++;
    stat_count_incr(customer_in_shop_counter);
    printf("DEBUG: %f customer %d enters shop\n", timestamp(), customer_num);
    if (remaining_seats == 0)
    {
        printf("DEBUG: %f customer %d leaves shop\n", timestamp(), customer_num);
        fullshop_visit ++;
        stat_count_decr(customer_in_shop_counter);
        pthread_mutex_unlock(&m);
        return;
    }
    stat_timer_start(timer);
    remaining_seats --;
    pthread_cond_signal(&customer_waiting);
    pthread_cond_wait(&haircut, &m);
    stat_count_incr(customer_in_barber_chair_counter);
    remaining_seats ++;
    printf("DEBUG: %f customer %d starts haircut\n", timestamp(), customer_num);
    sleep_exp(HAIRCUT_TIME, &m);
    pthread_cond_signal(&barber_seat_occupied);
    stat_count_decr(customer_in_barber_chair_counter);
    stat_timer_stop(timer);
    stat_count_decr(customer_in_shop_counter);
    printf("DEBUG: %f customer %d leaves shop\n", timestamp(), customer_num);

    pthread_mutex_unlock(&m);
}

/* Threads which call these methods. Note that the pthread create
 * function allows you to pass a single void* pointer value to each
 * thread you create; we actually pass an integer (the customer number)
 * as that argument instead, using a "cast" to pretend it's a pointer.
 */

/* the customer thread function - create 10 threads, each of which calls
 * this function with its customer number 0..9
 */
void *customer_thread(void *context)
{
    int customer_num = (int)context;

    /* your code goes here */
    while (1)
    {
        sleep_exp(CUSTOMER_SLEEPING_TIME, NULL);
        // printf("DEBUG: At %f, customer %d is entering the barbershop.\n", timestamp(), customer_num);
        customer(customer_num);
        // printf("DEBUG: At %f, customer %d has left the barbershop.\n", timestamp(), customer_num);
        // sleep_exp(SATISFIED_WITH_HAIR_TIME, &m);
    }

    return 0;
}

/*  barber thread
 */
void *barber_thread(void *context)
{
    // printf("Executing barber thread.\n");
    barber(); /* never returns */
    return 0;
}

void q2(void)
{
    /* to create a thread:
        pthread_t t;
        pthread_create(&t, NULL, function, argument);
       note that the value of 't' won't be used in this homework
    */

    /* your code goes here */
    pthread_t barber;
    pthread_create(& barber, NULL, barber_thread, NULL);

    pthread_t customers[TOTAL_CUSTOMERS];
    int i;
    for (i = 0 ; i < TOTAL_CUSTOMERS ; i ++){
        pthread_create(& customers[i], NULL, customer_thread, (void *) i);
    }

    wait_until_done();
}

/* For question 3 you need to measure the following statistics:
 *
 * 1. fraction of  customer visits result in turning away due to a full shop
 *    (calculate this one yourself - count total customers, those turned away)
 * 2. average time spent in the shop (including haircut) by a customer
 *     *** who does not find a full shop ***. (timer)
 * 3. average number of customers in the shop (counter)
 * 4. fraction of time someone is sitting in the barber's chair (counter)
 *
 * The stat_* functions (counter, timer) are described in the PDF.
 */

void q3(void)
{
    /* your code goes here */
    timer = stat_timer();
    customer_in_shop_counter = stat_counter();
    customer_in_barber_chair_counter = stat_counter();

    pthread_t barber;
    pthread_create(& barber, NULL, barber_thread, NULL);

    pthread_t customers[TOTAL_CUSTOMERS];
    int i;
    for (i = 0 ; i < TOTAL_CUSTOMERS ; i ++){
        pthread_create(& customers[i], NULL, customer_thread, (void *) i);
    }

    wait_until_done();

    printf("Total visits is: %d, fullshop visit is: %d, fraction is: %f\n", total_visit, fullshop_visit, 1.0 * fullshop_visit / total_visit);
    printf("The average time spending in barbershop is %f.\n", stat_timer_mean(timer));
    printf("The average customers in barbershop is: %f\n", stat_count_mean(customer_in_shop_counter));
    printf("The fraction of customer sitting in barber chair is: %f\n", stat_count_mean(customer_in_barber_chair_counter));
}
