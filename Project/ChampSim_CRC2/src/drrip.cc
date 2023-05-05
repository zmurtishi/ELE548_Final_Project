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
#define PSELMAX 1023

uint32_t rrpv[LLC_SETS][LLC_WAYS];
uint32_t psel;
uint32_t mode;

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
    cout << "Initialize SRRIP replacement state" << endl;
    psel = (PSELMAX/2);
    mode = 0;
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
    if((set==0)||((set%64)==0)){
        if(hit == 0){
            psel = Si(psel, PSELMAX);
        }
        if(hit){
            rrpv[set][way] = 0;
        }
        else{
            rrpv[set][way] = RRPV_MAX-1;
        }
        return;

    }
    else if((set==1)||((set%65)==0)){
        if(hit == 0){
            psel = Sd(psel);
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
    if(psel > (PSELMAX/2)){
        mode = 1;
    }
    else{
        mode = 0;
    }
    if(mode){
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
    for(int i = 0; i<LLC_SETS; i++)
    {
        for(int j = 0; j<LLC_WAYS; j++)
        {
            cout<<rrpv[i][j];
        }
    }
}
