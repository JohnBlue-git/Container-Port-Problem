/*
Auther: John Blue
Time: 2023/1
Platform: CodeChef IDE
Object: utilizing pthread to simulate Container Port Problem
*/

//
// import libarary
//

// basic libarary
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <string.h>

// bool
#include <stdbool.h>

// pthread libarary
#include <unistd.h>
#include <pthread.h>
//#include <semaphore.h>

//
// shipping, port, route ... class
//

struct shipping {
    char* way;// name of the shipping (from A port to B port)
    unsigned int span;// period that would cost
};
typedef struct shipping shipping;
shipping* create_shipping(char* wy, unsigned int tim) {
    shipping* shp = malloc(sizeof(shipping));
    shp->way = wy;
    shp->span = tim;
    return shp;
}

struct porting {
    char* port;// name of the port
    unsigned int unit;// number of (un)loading units in the port
    unsigned int wait_time;// each period would cost when (un)loading
    unsigned int queue_head, queue_tail;

    // pthread locks
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};
typedef struct porting porting;
porting* create_porting(char* pt, unsigned int num, unsigned int tim) {
    porting* ld = malloc(sizeof(porting));
    ld->port = pt;
    ld->unit = num;
    ld->wait_time = tim;
    ld->queue_head = 0;
    ld->queue_tail = 0;
    pthread_mutex_init(&(ld->lock), 0);
    pthread_cond_init(&(ld->cond), 0);
    return ld;
}

// class for storing ship, loading port, unloading port, ...
struct Route {
    char* ship;// name of ship
    shipping* way;// shipping route object
    porting* loading_port;// port object
    porting* unloading_port;// port object
    unsigned int deadline;// dealine
};
typedef struct Route Route;
Route* create_Route(char* shp, shipping* oc, porting* ld, porting* uld, unsigned int ddl) {
    Route* rt = malloc(sizeof(Route));
    rt->ship = shp; 
    rt->way = oc;
    rt->loading_port = ld;
    rt->unloading_port = uld;
    rt->deadline = ddl;
    return rt;
}

//
// thread class
//

void* port_thread(void* data) {
    // define variables
    porting* prt = (porting*)data;
    unsigned int ret;
    
    //
    // critical section
    //
    pthread_mutex_lock(&(prt->lock));

    unsigned int queue_me = prt->queue_tail++;// drawing ticket
    while (prt->unit < 0 || queue_me != prt->queue_head)
    {
        // it will release mutex lock and wait for signal or broadcast
        // FIFO policy
        pthread_cond_wait(&(prt->cond), &(prt->lock));
    }
    prt->unit--;// deduct number of available units

    pthread_mutex_unlock(&(prt->lock));
    //
    // critical section end
    //
    
    //
    // parallel working period
    //
    srand(time(NULL));
    ret = prt->wait_time + rand() % 3;
    usleep(ret);
    
    //
    // critical section
    //
    pthread_mutex_lock(&(prt->lock));
    
    prt->unit++;// reload number of available units
    prt->queue_head++;// de-queue

    // to signal or broadcast?
    //http://www.qnx.com/developers/docs/qnxcar2/index.jsp?topic=%2Fcom.qnx.doc.neutrino.getting_started%2Ftopic%2Fs1_procs_Condvar_signal_vs_broadcast.html
    pthread_cond_broadcast(&(prt->cond));
    //pthread_cond_signal(&(prt->cond));
    
    pthread_mutex_unlock(&(prt->lock));
    //
    // critical section end
    //
    
    // return
    //pthread_exit(ret);// fail many time why @@
    return (void*)ret;
}

void* shipping_thread(void* data) {
    unsigned int* span = (unsigned int*)data;
    unsigned int ret;
    
    srand(time(NULL));
    ret = *span + rand() % 3;
    usleep(ret);
    
    return (void*)ret;
}

unsigned int succ = 0;
unsigned int delay = 0;

void* ship_thread(void* data) {
    // define variables
    Route** mission = (Route**)data;
    pthread_t th;
    unsigned int Ret = 0;
    unsigned int ret = 0;
    
    //
    // loading port period
    //
    //printf("Ship %s at Port %s : loading\n", mission[0]->ship, mission[0]->loading_port->port);
    /* not work many times @@
    pthread_create(&th, NULL, port_thread, (void*)mission[0]->loading_port);
    pthread_join(th, (void**)&ret);
    if (ret == 0) {
        printf("Ship %s : thread fail @@ (1)\n", mission[0]->ship);
        return NULL;
    }*/
    while (ret == 0) {
        pthread_create(&th, NULL, port_thread, (void*)mission[0]->loading_port);
        pthread_join(th, (void**)&ret);
    }
    Ret += (unsigned int)ret;
    printf("Ship %s at Port %s : departing\n", mission[0]->ship, mission[0]->loading_port->port);

    //
    // shipping period
    //
    //printf("Ship %s on Way %s : sailing\n", mission[0]->ship, mission[0]->way->way);
    pthread_create(&th, NULL, shipping_thread, (void*)&(mission[0]->way->span));
    pthread_join(th, (void**)&ret);
    Ret += (unsigned int)ret;
    printf("Ship %s on Way %s : reaching\n", mission[0]->ship, mission[0]->way->way);
    
    //
    // unloading port period
    //
    //printf("Ship %s at Port %s : unloading\n", mission[0]->ship, mission[0]->unloading_port->port);
    pthread_create(&th, NULL, port_thread, (void*)mission[0]->unloading_port);
    pthread_join(th, (void**)&ret);
    Ret += (unsigned int)ret;
    printf("Ship %s at Port %s : shipping finished\n", mission[0]->ship, mission[0]->unloading_port->port);

    //
    // count sucess or delay
    //
    bool check = Ret > mission[0]->deadline;// 1 delay 0 sucess
    if (check) {
        //printf("Ship %s delay\n", mission[0]->ship);
        delay++;
    }
    else {
        //printf("Ship %s success\n", mission[0]->ship);
        succ++;
    }
}

//
// main function
//

void sub_main(unsigned int ship_span, unsigned int port_span, unsigned int deadline, unsigned int port_A_unit, unsigned int port_B_unit) {
    // define ships number
    unsigned int ship_N = 12;
    char ship_name[12][3];

    // define route, ports
    shipping* A_to_B = create_shipping("A_to_B", ship_span);// shipping time
    porting* A = create_porting("A", port_A_unit, port_span);// port A
    porting* B = create_porting("B", port_B_unit, port_span);// port B
    // mission_list
    // [0]{[0]Route*, [1]Route*, ...}, [1]{[0]...}, ...
    Route*** mission_list = malloc(sizeof(Route**) * ship_N);
    for (int i = 0; i < ship_N; i++) {
        // give ship name (00, 01, to ...)
        if (i < 10) {
            ship_name[i][0] = '0';
        }
        else if (i < 20) {
            ship_name[i][0] = '1';
        }
        else if (i < 30) {
            ship_name[i][0] = '2';
        }
        else {
            ship_name[i][0] = '3';
        }
        ship_name[i][1] = i % 10 + '0';
        ship_name[i][2] = '\0';
        
        // mission_list
        mission_list[i] = malloc(sizeof(Route*) * 1);
        mission_list[i][0] = create_Route(ship_name[i], A_to_B, A, B, deadline);
    }
    
    // threads start
    pthread_t t[ship_N];
    for (int i = 0; i < ship_N; i++) {
        pthread_create(&t[i], NULL, ship_thread, (void*)mission_list[i]);
    }
    
    // wait till every thread
    for (int i = 0; i < ship_N; i++) {
        pthread_join(t[i], NULL);
    }

    // print results
    printf("...\n");
    printf("Ship unit: 12; port A unit: %u; port B unit: %u\n", port_A_unit, port_B_unit);
    printf("Sucess: %u; Delayed: %u\n\n\n", succ, delay);
    succ = 0;    delay = 0;

    
    // free allocated memory
    free(A_to_B);
    free(A);
    free(B);
    for(int i = 0; i < 3; i++) {
        free(mission_list[i]);
    }
    free(mission_list);
}

int main()
{
    // define variables
    unsigned int ship_span = 9;
    unsigned int port_span = 3;
    unsigned int deadline = ship_span + port_span + port_span + 1;
    unsigned int port_A_unit = 0;
    unsigned int port_B_unit = 0;
    
    // assign port units
    port_A_unit = 4;
    port_B_unit = 4;
    // sub main
    sub_main(ship_span, port_span, deadline, port_A_unit, port_B_unit);

    // assign port units
    port_A_unit = 2;
    port_B_unit = 2;
    // sub main
    sub_main(ship_span, port_span, deadline, port_A_unit, port_B_unit);

    return 0;
}
