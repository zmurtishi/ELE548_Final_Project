////////////////////////////////////////////
//                                        //
//        LRU replacement policy          //
//     Jinchun Kim, cienlux@tamu.edu      //
//                                        //
////////////////////////////////////////////

#include "../inc/champsim_crc2.h"

#define NUM_CORE 4
#define LLC_SETS NUM_CORE*2048
#define LLC_WAYS 16
#define RRPV_MAX 3
#define PSEL_MAX 1023
#define SAMPLE_SETS 64

uint32_t rrpv[LLC_SETS][LLC_WAYS];
uint32_t psel[NUM_CORE];
uint32_t mode[NUM_CORE];
uint32_t s_sets;

// initialize replacement state

int Rng(float p){
    float rnum = (float)rand()/RAND_MAX;
    if(rnum < p){
        return 1;
    }
    else{
        return 0;
    }
}

uint32_t Sd(uint32_t n){
    uint32_t temp;
    if(n > 0){
        temp = n -1;
        return temp;
    }
    return n;
}

uint32_t Si(uint32_t n, uint32_t m){
    uint32_t temp;
    if(n < m){
        temp = n + 1;
        return temp;
    }
    return n;
}

void InitReplacementState()
{
    s_sets = LLC_SETS/SAMPLE_SETS;
    //cout << "Initialize SRRIP replacement state" << end;
    for (int k=0; k<NUM_CORE; k++){
        psel[k] = PSEL_MAX/2;
        mode[k] = 0;
    }
    for (int i=0; i<LLC_SETS; i++) {
        for (int j=0; j<LLC_WAYS; j++) {
            rrpv[i][j] = 3;
        }
    }
}

// find replacement victim
// return value should be 0 ~ 15 or 16 (bypass)
uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    while(1){
        for(int i=0; i<LLC_WAYS; i++){
            if(rrpv[set][i] == RRPV_MAX){
                return i;
            }
        }
        for(int j=0; j<LLC_WAYS; j++){
            rrpv[set][j]++;
        }
    }
}

// called on every cache hit and cache fill
void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    if((set%s_sets)==0){
        if(hit == 0){
            psel[cpu] = Si(psel[cpu], PSEL_MAX);
        }
        if(hit){
            rrpv[set][way] = 0;
        }
        else{
            rrpv[set][way] = RRPV_MAX-1;
        }
        return;

    }
    else if(set%(s_sets+1)==0){
        if(hit == 0){
            psel[cpu] = Sd(psel[cpu]);
        }
        if(hit){
            rrpv[set][way] = 0;
        }
        else{
            if(Rng(0.96875)){
                rrpv[set][way] = RRPV_MAX-1;
            }
            else{
                rrpv[set][way] = RRPV_MAX-2;
            }
        }
        return;
    }
    if(psel[cpu] > (PSEL_MAX/2)){
        mode[cpu] = 1;
    }
    else{
        mode[cpu] = 0;
    }
    if(mode[cpu]){
        if(hit){
            rrpv[set][way] = 0;
        }
        else{
            if(Rng(0.96875)){
                rrpv[set][way] = RRPV_MAX-1;
            }
            else{
                rrpv[set][way] = RRPV_MAX-2;
            }
        }
    }
    else{
        if(hit){
            rrpv[set][way] = 0;
        }
        else{
            rrpv[set][way] = RRPV_MAX-1;
       }
    }
}

// use this function to print out your own stats on every heartbeat 
void PrintStats_Heartbeat()
{

}

// use this function to print out your own stats at the end of simulation
void PrintStats()
{
    uint32_t sum = 0;
    float total_ipc = 0;
    for(int i=0; i<4; i++){
        sum += get_instr_count(i);
    }
    sum = sum/4;
    total_ipc = (float)sum/get_cycle_count();
    cout<<"Process 1 IPC: "<<get_instr_count(0)/get_cycle_count()<<"\n";
    cout<<"Process 2 IPC: "<<get_instr_count(1)/get_cycle_count()<<"\n";
    cout<<"Process 3 IPC: "<<get_instr_count(2)/get_cycle_count()<<"\n";
    cout<<"Process 4 IPC: "<<get_instr_count(3)/get_cycle_count()<<"\n";
    cout<<"Cumulative IPC: "<<total_ipc<<"\n";
}
