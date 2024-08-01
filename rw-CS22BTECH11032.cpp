#include <iostream>
#include <string>
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <chrono>
#include <sstream>
#include <unistd.h>
#include <semaphore.h>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <ctime>
#include <thread>
#include <random>
#include <sys/time.h>
#define inputFile "inp-params.txt"
#define outputFile "RW-log.txt"
#define avgDelayOutputFile "Average_time.txt"

using namespace std;

ofstream outfile(outputFile);
ofstream avgfile(avgDelayOutputFile); 


const int MAX_DELAY = 2000;

int nw, nr, kw, kr;
double μCS, μRem;
int randCSTime, randRemTime;
// vectors to store the distributions
exponential_distribution<double>db1;
exponential_distribution<double>db2;
// randomly generating the exponential distribution
random_device rd1;
mt19937 gen1(rd1());
random_device rd2;
mt19937 gen2(rd2());

//vectors to store waiting time
vector<vector<long long>> avgWriterTime;
vector<vector<long long>> avgReaderTime;


// Structure to store the reader-writer lock
typedef struct _rwlock{
    int readers_count;
    int writers_count;
    sem_t writers_lock;
    sem_t readers_lock;
    sem_t writers_to_readers;
    sem_t resource_lock;

} rwlock;

// Reader-writer lock
rwlock rw;

//semaphore to avoid synchronization issues while printing into output file
sem_t print_lock;
// Function to initialize the reader-writer lock
void rw_lock_init(rwlock *rw){
    rw->readers_count = 0;
    rw->writers_count = 0;
    sem_init(&rw->writers_lock, 0, 1);
    sem_init(&rw->readers_lock, 0, 1);
    sem_init(&rw->writers_to_readers, 0, 1);
    sem_init(&rw->resource_lock, 0, 1);
}

// Function to acquire write lock
void rw_lock_writer_acquire(rwlock *rw){
    sem_wait(&rw->writers_lock);
    rw->writers_count++;
    if(rw->writers_count==1){
        sem_wait(&rw->writers_to_readers);  
    }
    sem_post(&rw->writers_lock);
    sem_wait(&rw->resource_lock);
}
// Function to release write lock
void rw_lock_writer_release(rwlock *rw){
    sem_post(&rw->resource_lock);
    sem_wait(&rw->writers_lock);
    rw->writers_count--;
    if(rw->writers_count==0){
        sem_post(&rw->writers_to_readers);
    }
    sem_post(&rw->writers_lock);
}

// Function to acquire read lock
void rw_lock_reader_acquire(rwlock *rw){
    sem_wait(&rw->writers_to_readers);
    sem_wait(&rw->readers_lock);
    rw->readers_count++;
    if(rw->readers_count==1){
        sem_wait(&rw->resource_lock);
    }
    sem_post(&rw->readers_lock);
    sem_post(&rw->writers_to_readers);
}

// Function to release read lock
void rw_lock_reader_release(rwlock *rw){
    sem_wait(&rw->readers_lock);
    rw->readers_count--;
    if(rw->readers_count==0){
        sem_post(&rw->resource_lock);
    }
    sem_post(&rw->readers_lock);
}
// Writer thread function
void* writer(void* arg) {
    int id = *static_cast<int*>(arg);
    for (int i = 0; i < kw; i++) {

        //getting the request time  
        time_t request_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
        stringstream output;


        auto start = std::chrono::high_resolution_clock::now();
        sem_wait(&print_lock);
        output << i << "th CS request by Writer Thread " << id << " at " << ctime(&request_time)<<endl;
        outfile<<output.str();
        sem_post(&print_lock);

        // Acquire write lock
        rw_lock_writer_acquire(&rw);

        auto end = std::chrono::high_resolution_clock::now();
        //storing the waiting time for each writer
        long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        avgWriterTime[id][i]=milliseconds;


        //getting the entry time
        time_t entry_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
        output.str("");
        sem_wait(&print_lock);  
        output<<i<<"th CS entry by Writer Thread "<<id<<" at "<<ctime(&entry_time)<<endl;
        outfile<<output.str();
        sem_post(&print_lock);
        


        //making the thread sleep
        randCSTime = db1(gen1);
        this_thread::sleep_for(chrono::milliseconds(randCSTime));

        time_t exit_time = chrono::system_clock::to_time_t(chrono::system_clock::now());

        output.str("");
        sem_wait(&print_lock);
        output<<i<<"th CS exit by Writer Thread "<<id<<" at "<<ctime(&exit_time)<<endl;
        outfile<<output.str();
        sem_post(&print_lock);

        rw_lock_writer_release(&rw);

        // Simulate a thread executing in remainder section
        randRemTime = db2(gen2);
        this_thread::sleep_for(chrono::milliseconds(randRemTime));
    }
    return NULL;
}

// Reader thread function
void* reader(void* arg) {
        int id = *static_cast<int*>(arg);
    for (int i = 0; i < kr; i++) {
        time_t request_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
        auto start = std::chrono::high_resolution_clock::now();
        stringstream output;
        sem_wait(&print_lock);
        output << i << "th CS request by Reader Thread " << id << " at " << ctime(&request_time)<<endl;
        outfile<<output.str();
        sem_post(&print_lock);


        rw_lock_reader_acquire(&rw);

        time_t entry_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
        auto end = std::chrono::high_resolution_clock::now();
        long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();    
        avgReaderTime[id][i]=milliseconds;
        
        //clear the stringstream
        output.str("");
        sem_wait(&print_lock);
        output << i << "th CS Entry by Reader Thread " << id << " at " << ctime(&entry_time)<<endl;
        outfile<<output.str();
        sem_post(&print_lock);    


        randCSTime = db1(gen1);
        this_thread::sleep_for(chrono::milliseconds(randCSTime));


        time_t exit_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
        output.str(""); 
        sem_wait(&print_lock);
        output << i << "th CS Exit by Reader Thread " << id << " at " <<ctime(&exit_time)<<endl;
        outfile<<output.str();
        sem_post(&print_lock);

        rw_lock_reader_release(&rw);

        // Simulate a thread executing in remainder section
        randRemTime = db2(gen2);
        this_thread::sleep_for(chrono::milliseconds(randRemTime));
    }
    pthread_exit(0);
}
//function to print average times into output file
void printAverageTimes(){
    for(int i=1;i<=nw;i++){
        long long avg=0;
        for(int j=0;j<kw;j++){
            avg+=avgWriterTime[i][j];
        }
        avg=avg/kw;
        avgfile<<"Average wait time for Writer Thread "<<i<<" at "<<avg<<" milli seconds"<<endl;
    }
    for(int i=1;i<=nr;i++){
        long long avg=0;
        for(int j=0;j<kr;j++){
            avg+=avgReaderTime[i][j];
        }
        avg=avg/kr;
        avgfile<<"Average wait time for Reader Thread "<<i<<" at "<<avg<<" milli seconds"<<endl;
    }
}

int main() {
    // Open input file
    ifstream file(inputFile);
    sem_init(&print_lock, 0, 1);
    if (!file.is_open()) {
        cout << inputFile << endl;
        cerr << "Error: Unable to open input file." << endl;
        return 1;
    }
    // Read parameters from input file
    file >> nw >> nr >> kw >> kr >> μCS >> μRem;
    cout<<nw<<" "<<nr<<" "<<kw<<" "<<kr<<" "<<μCS<<" "<<μRem<<endl;
    file.close();

    db1 = exponential_distribution<double>((double)(1.0 / μCS));
    db2 = exponential_distribution<double>((double)(1.0 / μRem));

    // Initialize the reader-writer lock
    rw_lock_init(&rw);

    vector<pthread_t> writer_threads(nw+1),reader_threads(nr+1);
    vector<pthread_attr_t> att_writers(nw+1),att_readers(nr+1);
    vector<int> ii(nw+1),ii2(nr+1);
    avgWriterTime=vector<vector<long long>>(nw+1,vector<long long>(kw+1,0));
    avgReaderTime=vector<vector<long long>>(nr+1,vector<long long>(kr+1,0));



    // creating writer threads
    for (int i = 1; i <= nw; i++) {
        //argument to pass to the thread
        ii[i]=i;
        // Initialize the thread attributes
        pthread_attr_init(&att_writers[i]);
        // Create the writer thread
        pthread_create(&writer_threads[i], &att_writers[i], writer, &ii[i]);
    }

    // creating reader threads
    for (int i = 1; i <= nr; i++) {
        //argument to pass to the thread
        ii2[i]=i;
        // Initialize the thread attributes
        pthread_attr_init(&att_readers[i]);
        // Create the reader thread
        pthread_create(&reader_threads[i], &att_readers[i], reader, &ii2[i]);
    }

    // Join writer threads
    for (int i = 1; i <= nw; i++) {
        pthread_join(writer_threads[i], NULL);
    }
    // Join reader threads
    for (int i = 1; i <= nr; i++) {
        pthread_join(reader_threads[i], NULL);
    }

    //printing average waiting times
    printAverageTimes();
    //calculating average waiting time
    long long avgWriter=0,avgReader=0;
    for(int i=1;i<=nw;i++){
        for(int j=0;j<kw;j++){
            avgWriter+=avgWriterTime[i][j];
        }
    }
    for(int i=1;i<=nr;i++){
        for(int j=0;j<kr;j++){
            avgReader+=avgReaderTime[i][j];
        }
    }
    avgWriter=avgWriter/(nw*kw);
    avgReader=avgReader/(nr*kr);
    cout<<"Average waiting time of writers is "<<avgWriter<<" milli seconds"<<endl;
    cout<<"Average waiting time of readers is "<<avgReader<<" milli seconds"<<endl;
    //adding waiting times to avgfile
    //calculating worst case waiting time
    avgfile<<"Average waiting time of writers is "<<avgWriter<<" milli seconds"<<endl;
    avgfile<<"Average waiting time of readers is "<<avgReader<<" milli seconds"<<endl;
    long long worstWriter=0,worstReader=0;
    for(int i=1;i<=nw;i++){
        for(int j=0;j<kw;j++){
            if(avgWriterTime[i][j]>worstWriter)
                worstWriter=avgWriterTime[i][j];
        }
    }
    for(int i=1;i<=nr;i++){
        for(int j=0;j<kr;j++){
            if(avgReaderTime[i][j]>worstReader)
                worstReader=avgReaderTime[i][j];
        }
    }
    cout<<"Worst case waiting time of writers is "<<worstWriter<<" milli seconds"<<endl;
    cout<<"Worst case waiting time of readers is "<<worstReader<<" milli seconds"<<endl;
    //adding worst case waiting times to avgfile
    avgfile<<"Worst case waiting time of writers is "<<worstWriter<<" milli seconds"<<endl;
    avgfile<<"Worst case waiting time of readers is "<<worstReader<<" milli seconds"<<endl;
    return 0;
}
