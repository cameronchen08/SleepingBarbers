/* Cameron Chen
   CSS 503 - Project 2: Sleeping Barbers
   Shop Class Header File
*/

#ifndef SHOP_H
#define SHOP_H
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
#include <vector>
using namespace std;

#define kDefaultNumChairs 3
#define kDefaultBarbers 1

class Shop {
public: 
    Shop();                                                    // default constructor
    Shop(int numChairs, int numBarbers);                       // parameterized constructor
    ~Shop();                                                   // destructor  

    int visitShop(int id);                                     // customer visits shop   
    void leaveShop(int customerID, int barberID);              // customer leaves shop
    void helloCustomer(int barberID);                          // barber serves customer  
    void byeCustomer(int barberID);                            // barber completes service
    int getCustomerDrops() const;                              // get number of customers that left without service

private:
    const int maxWaitingCustomers;                             // max number of waiting chairs
    int customerDrops;                                         // number of customers that left without service
    pthread_mutex_t mutex;                                     // mutex for critical section

    queue<int> waitingChairs;                                  // queue of customers waiting for service
    queue<pthread_cond_t> waitingChairConditions;              // conditions for waiting chairs

    vector<int> serviceChairs;                                 // vector of service chairs                                                       
    vector<pthread_cond_t> serviceChairConditions;             // conditions for service chairs

    void init(int numBarbers);                                 // initialize barber conditions
    string int2string(int i);                                  // convert int to string
    void print(int customer, bool isBarber, string message);   // print message
};
#endif