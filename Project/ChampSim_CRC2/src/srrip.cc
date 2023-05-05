////////////////////////////////////////////
//                                        //
//        LRU replacement policy          //
//     Jinchun Kim, cienlux@tamu.edu      //
//                                        //
////////////////////////////////////////////

#include "../inc/champsim_crc2.h"

#define NUM_CORE 1
#define LLC_SETS NUM_CORE*2048
#define LLC_WAYS 16
#define RRPV_MAX 3
uint32_t rrpv[LLC_SETS][LLC_WAYS];
uint64_t dummy;
// initialize replacement state
void InitReplacementState()
{
    cout << "Initialize SRRIP replacement state" << endl;

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
    dummy = PC;
    if(hit){
        rrpv[set][way] = 0;
    }
    else{
        rrpv[set][way] = RRPV_MAX-1;
    }
}

// use this function to print out your own stats on every heartbeat 
void PrintStats_Heartbeat()
{

}

// use this function to print out your own stats at the end of simulation
void PrintStats()
{
    cout<<dummy;
}
