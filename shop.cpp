/* Cameron Chen
   CSS 503 - Project 2: Sleeping Barbers
   Shop Class Implementation 
*/

#include "shop.h"

// default constructor
Shop::Shop() : maxWaitingCustomers(kDefaultNumChairs), serviceChairs(0), customerDrops(0) {
    init(kDefaultBarbers);
}

// parameterized constructor
// chairs(numBarbers, -1) - # of barber chairs with no customers sitting in them
Shop::Shop(int numChairs, int numBarbers) : maxWaitingCustomers((numChairs > 0) ? numChairs : kDefaultNumChairs), serviceChairs(numBarbers, -1), customerDrops(0) {
    init(numBarbers);
}

// destructor
Shop::~Shop() {
    // destroy mutex
    pthread_mutex_destroy(&mutex);
    
    // destroy condition variables
    for (int i = 0; i < serviceChairConditions.size(); i++) {
        pthread_cond_destroy(&serviceChairConditions[i]);
    }
    
    for (int i = 0; i < waitingChairConditions.size(); i++) {
        pthread_cond_destroy(&waitingChairConditions.front());
    }
}

// ----------------------------------------------------------------------------------------------------------------------------------------
// called by customer thread when entering the shop
int Shop::visitShop(int id) {
    pthread_mutex_lock(&mutex);
 
    // if all chairs are full then leave shop
    if (waitingChairs.size() == maxWaitingCustomers) {
        print(id, false, "leaves the shop because of no available waiting chairs.");
        ++customerDrops;
        pthread_mutex_unlock(&mutex);
        return -1; // customer leaves without service 
    }
 
    // if all barbers are busy and waiting chairs are available then wait
    // serviceChairs[i] == -1 means barber is not busy
    int firstEmptyChair = -1;
    for (int i = 0; i < serviceChairs.size(); i++) {
        
        // find the first empty chair
        if (serviceChairs[i] == -1) {
            
            // set first empty chair to index of first empty chair (chair ID)
            firstEmptyChair = i; 
            break;
        }
    }
    // all service chairs are full
    if (firstEmptyChair == -1) {
        // take a waiting chair
        waitingChairs.push(id);
        
        // create and initialize a condition variable for the waiting chair
        waitingChairConditions.push(PTHREAD_COND_INITIALIZER);
        pthread_cond_init(&waitingChairConditions.back(), NULL); 

        print(id, false, "takes a waiting chair. # waiting seats available = " + int2string(maxWaitingCustomers - waitingChairs.size()));
        
        // wait for barber to be available
        pthread_cond_wait(&waitingChairConditions.back(), &mutex);
        
        // remove customer from waiting queue
        waitingChairs.pop();
        
        // remove condition variable from waiting chair conditions
        pthread_cond_destroy(&waitingChairConditions.front());
        waitingChairConditions.pop();

        // find the first empty chair again
        for (int i = 0; i < serviceChairs.size(); i++) {
            if (serviceChairs[i] == -1) {
                firstEmptyChair = i;
                break;
            }
        }
    }

    if (firstEmptyChair == -1) {
        std::cerr << "Error: No empty service chairs available." << std::endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    // get barber whose id is barberID - barberID is the id of the service chair
    int barberID = firstEmptyChair;

    // move customer to service chair - put customer in service chair vector
    print(id, false, "moves to the service chair. # waiting seats available = " + int2string(maxWaitingCustomers - waitingChairs.size()));
    serviceChairs[firstEmptyChair] = id;

    // wake up the barber just in case if he is sleeping - wake up a specific barber
    pthread_cond_signal(&serviceChairConditions[firstEmptyChair]);

    pthread_mutex_unlock(&mutex);
    return barberID;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
//called by customer thread after service is done
void Shop::leaveShop(int customerID, int barberID) {
    pthread_mutex_lock(&mutex);
    
    print(customerID, false, "waits for barber " + int2string(barberID) + " to be done with haircut.");

    // wait for barber to be done with service
    pthread_cond_wait(&serviceChairConditions[barberID], &mutex);

    print(customerID, false, "pays barber " + int2string(barberID) + " for the service.");
    
    // signal to barber that customer is done with service
    pthread_cond_signal(&serviceChairConditions[barberID]);

    // customer says goodbye barber
    print(customerID, false, "says good-bye to barber " + int2string(barberID) + ".");

    // remove customer from service chair
    serviceChairs[barberID] = - 1;
    
    pthread_mutex_unlock(&mutex);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
// called by barber thread when there are no customers
void Shop::helloCustomer(int barberID) {
    pthread_mutex_lock(&mutex);
 
    // if barber is not busy and no customers are waiting then barber sleeps
    // waitingChair queue is empty and chair vector has all -1
    if (waitingChairs.empty() && serviceChairs[barberID] == -1) {
        print(barberID, true, "sleeps because of no customers.");

        // wait for a customer to arrive
        pthread_cond_wait(&serviceChairConditions[barberID], &mutex);
    }

    // if barber is not busy and there are customers waiting then barber wakes up
    if (serviceChairs[barberID] == -1) {
        pthread_cond_wait(&serviceChairConditions[barberID], &mutex);
    }
    print(barberID, true, "starts a hair-cut service for customer " + int2string(serviceChairs[barberID]));
    
    pthread_mutex_unlock(&mutex);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
// called by barber thread when service is done
void Shop::byeCustomer(int barberID) {
    pthread_mutex_lock(&mutex);

    if (barberID < 0 || barberID >= serviceChairs.size()) {
        std::cerr << "Error: Invalid barber ID." << std::endl;
        pthread_mutex_unlock(&mutex);
        return;
    }
    print(barberID, true, "says he's done with a hair-cut service for customer " + int2string(serviceChairs[barberID]));\
    
    // wake up the customer in service chair
    pthread_cond_signal(&serviceChairConditions[barberID]);

    // barber waits for customer to pay
    pthread_cond_wait(&serviceChairConditions[barberID], &mutex);

    print(barberID, true, "calls in another customer.");
    
    if (!waitingChairs.empty()) {
        
        // barber wakes up the next customer in waiting chair
        pthread_cond_signal(&waitingChairConditions.front());
    }
    
    pthread_mutex_unlock(&mutex);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
int Shop::getCustomerDrops() const {
    return customerDrops;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void Shop::init(int numBarbers) {
    pthread_mutex_init(&mutex, NULL);

    // initialize barber conditions
    serviceChairs.resize(numBarbers, -1);
    for (int i = 0; i < numBarbers; i++) {
        serviceChairConditions.push_back(PTHREAD_COND_INITIALIZER);
        pthread_cond_init(&serviceChairConditions[i], NULL);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------------------
string Shop::int2string(int i) {
    stringstream out;
    out << i;
    return out.str();
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void Shop::print(int person, bool isBarber, string message) {
    if (isBarber) {
        cout << "barber   [" << person << "]: " << message << endl; 
    } else {
        cout << "customer [" << person << "]: " << message << endl;
    }
    
}


