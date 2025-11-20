#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include "semaphore_class.h"
using namespace std;

// Thread counts
const int NUM_READERS = 5;
const int NUM_WRITERS = 5;
const int NUM_PHILOS = 5;

// Semaphores and shared variables

// Readers–Writers (No Starve)
Semaphore ns_mutex(1);
Semaphore ns_roomEmpty(1);
Semaphore ns_turnstile(1);
int ns_readCount = 0;

// Readers–Writers (Writer Priority)
Semaphore wp_mutex(1);
Semaphore wp_writeLock(1);
Semaphore wp_readerLock(1);
int wp_readCount = 0;

// Dining Philosophers – Shared Semaphores
Semaphore tableLimit(NUM_PHILOS - 1);  // Used in solution 1
vector<Semaphore*> forks;

// Thread safe print function
void safe_print(const string& s) {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);
    cout << s << endl;
    pthread_mutex_unlock(&lock);
}

// No-starve readers/writers implementation
void* reader_ns(void* id_ptr) {
    int id = *(int*)id_ptr;

    while (true) { // reader loop
    
        ns_turnstile.wait(); // enter turnstile
        ns_turnstile.signal(); // exit turnstile

        ns_mutex.wait(); // protect readCount
        ns_readCount++; // increment reader count
        if (ns_readCount == 1) // first reader
            ns_roomEmpty.wait();
        ns_mutex.signal(); // release protection

        safe_print("Reader " + to_string(id) + ": reading");
        usleep(200000);

        ns_mutex.wait(); // protect readCount
        ns_readCount--; // decrement reader count
        if (ns_readCount == 0)
            ns_roomEmpty.signal();
        ns_mutex.signal(); // release protection

        usleep(100000);
    }
}

void* writer_ns(void* id_ptr) { // writer loop
    int id = *(int*)id_ptr; // get writer id

    while (true) {
        ns_turnstile.wait(); // enter turnstile

        ns_roomEmpty.wait(); // wait until room is empty
        safe_print("Writer " + to_string(id) + ": writing");
        usleep(300000); // simulate writing
        ns_roomEmpty.signal();

        ns_turnstile.signal(); // exit turnstile
        usleep(200000);
    }
}

void no_starve() { // start no-starve solution
    pthread_t readers[NUM_READERS], writers[NUM_WRITERS]; // thread arrays
    int ids[NUM_READERS > NUM_WRITERS ? NUM_READERS : NUM_WRITERS]; // id array

    for (int i = 0; i < NUM_READERS; i++) { // create reader threads
        ids[i] = i; 
        pthread_create(&readers[i], NULL, reader_ns, &ids[i]);
    }

    for (int i = 0; i < NUM_WRITERS; i++) { // create writer threads
        ids[i] = i;
        pthread_create(&writers[i], NULL, writer_ns, &ids[i]);
    }

    for (;;) sleep(1); // keep main thread alive
}

// Writer priority readers/writers implementation
void* reader_wp(void* id_ptr) {
    int id = *(int*)id_ptr;

    while (true) { // reader loop
        wp_readerLock.wait(); // block if writer is waiting
        wp_mutex.wait(); // protect readCount

        wp_readCount++; // increment reader count
        if (wp_readCount == 1) // first reader
            wp_writeLock.wait(); // block writers

        wp_mutex.signal(); // release protection
        wp_readerLock.signal(); // allow writers to proceed

        safe_print("Reader " + to_string(id) + ": reading");
        usleep(200000);

        wp_mutex.wait();
        wp_readCount--;
        if (wp_readCount == 0)
            wp_writeLock.signal();
        wp_mutex.signal();

        usleep(100000);
    }
}

void* writer_wp(void* id_ptr) {
    int id = *(int*)id_ptr;

    while (true) {
        wp_readerLock.wait(); // block new readers
        wp_writeLock.wait(); // wait until no readers

        safe_print("Writer " + to_string(id) + ": writing");
        usleep(300000); // simulate writing

        wp_writeLock.signal(); // release write lock
        wp_readerLock.signal(); // release reader lock

        usleep(200000); // simulate time between writes
    }
}

void writer_priority() { // run writer priority solution
    pthread_t readers[NUM_READERS], writers[NUM_WRITERS]; // thread arrays
    int ids[NUM_READERS > NUM_WRITERS ? NUM_READERS : NUM_WRITERS];

    for (int i = 0; i < NUM_READERS; i++) { // create reader threads
        ids[i] = i;
        pthread_create(&readers[i], NULL, reader_wp, &ids[i]);
    }

    for (int i = 0; i < NUM_WRITERS; i++) { // create writer threads
        ids[i] = i;
        pthread_create(&writers[i], NULL, writer_wp, &ids[i]);
    }

    for (;;) sleep(1); // keep main thread alive
}

// Dining Philosophers – Solution 1
// (Limit number of philosophers at table)
void* philosopher_sol1(void* id_ptr) {
    int id = *(int*)id_ptr; // get philosopher id
    int left = id; // left fork index
    int right = (id + 1) % NUM_PHILOS; // right fork index

    while (true) { // philosopher loop
        safe_print("Philosopher " + to_string(id) + ": thinking");
        usleep(200000); // simulate thinking

        tableLimit.wait(); // limit number of philosophers at table
        forks[left]->wait(); // pick up left fork
        forks[right]->wait(); // pick up right fork

        safe_print("Philosopher " + to_string(id) + ": eating");
        usleep(250000);

        forks[right]->signal(); // put down right fork
        forks[left]->signal(); // put down left fork
        tableLimit.signal();
    }
}

void dining1() { // run dining solution 1
    forks.clear(); // clear forks vector
    for (int i = 0; i < NUM_PHILOS; i++) // create forks
        forks.push_back(new Semaphore(1));

    pthread_t phil[NUM_PHILOS]; // philosopher threads
    int ids[NUM_PHILOS];

    for (int i = 0; i < NUM_PHILOS; i++) { // create philosopher threads
        ids[i] = i;
        pthread_create(&phil[i], NULL, philosopher_sol1, &ids[i]);
    }

    for (;;) sleep(1);
}

// Dining Philosophers — Solution 2
// (Asymmetric fork picking)
void* philosopher_sol2(void* id_ptr) {
    int id = *(int*)id_ptr; 
    int left = id;
    int right = (id + 1) % NUM_PHILOS;

    while (true) { // philosopher loop
        safe_print("Philosopher " + to_string(id) + ": thinking");
        usleep(200000);

        if (id % 2 == 0) { // even philosophers pick up right fork first
            forks[right]->wait();
            forks[left]->wait();
        } else { // odd philosophers pick up left fork first
            forks[left]->wait();
            forks[right]->wait();
        }

        safe_print("Philosopher " + to_string(id) + ": eating");
        usleep(250000);

        forks[left]->signal(); // put down left fork
        forks[right]->signal(); // put down right fork
    }
}

void dining2() { // run dining solution 2
    forks.clear(); // clear forks vector
    for (int i = 0; i < NUM_PHILOS; i++) // create forks
        forks.push_back(new Semaphore(1));

    pthread_t phil[NUM_PHILOS];
    int ids[NUM_PHILOS];

    for (int i = 0; i < NUM_PHILOS; i++) { // create philosopher threads
        ids[i] = i;
        pthread_create(&phil[i], NULL, philosopher_sol2, &ids[i]);
    }

    for (;;) sleep(1);
}

// main entry point
int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 1;
    }

    int problem = atoi(argv[1]); // specific problem to run

    switch (problem) {
        case 1: no_starve(); break;
        case 2: writer_priority(); break;
        case 3: dining1(); break;
        case 4: dining2(); break;
        default:
            cout << "Invalid argument (must be 1-4)\n";
            return 1;
    }
    return 0;
}
