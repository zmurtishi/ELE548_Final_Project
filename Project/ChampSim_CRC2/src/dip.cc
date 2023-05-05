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

uint32_t lru[LLC_SETS][LLC_WAYS];
uint32_t mode;
uint32_t psel;

int Si(int n, int max){
    if(n < max){
        n++;
	return n;
    }
    else{
	return n;
    }
}


int Sd(int n){
    if(n > 0){
	n--;
	return n;
    }
    else{
	return n;
    }
}


int Rng(float p){
    float rnum = (float)rand()/RAND_MAX;
    if(rnum < p){
        return 1;
    }
    else{
	return 0;
    }
}

// initialize replacement state
void InitReplacementState()
{
    mode = 0;
    psel = 512;
    cout << "Initialize LRU replacement state" << endl;

    for (int i=0; i<LLC_SETS; i++) {
        for (int j=0; j<LLC_WAYS; j++) {
            lru[i][j] = j;
        }
    }
}

// find replacement victim
// return value should be 0 ~ 15 or 16 (bypass)
uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    for (int i=0; i<LLC_WAYS; i++)
        if (lru[set][i] == (LLC_WAYS-1))
            return i;

    return 0;
}

// called on every cache hit and cache fill
void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    // update lru replacement state
    for (uint32_t i=0; i<LLC_WAYS; i++) {
        if (lru[set][i] < lru[set][way]) {
            lru[set][i]++;

            if (lru[set][i] == LLC_WAYS)
                assert(0);
        }
    }
    if((set == 0) | ((set % 64)==0)){
        lru[set][way] = 0;
        if(hit == 0){
            psel = Si(psel, 1023); //increment for LRU
        }
        return;
    }
    else if((set == 1) | ((set % 65)==0)){
        if(Rng(0.03125)){
            lru[set][way] = 0;
        }
        else{
            lru[set][way] = 15; // promote to the MRU position
        }
        if(hit == 0){
            psel = Sd(psel); //decrement for BIP
        }
        return;
    }
    if(psel > 512){
        mode = 1; //if MSB is 1, use BIP
    }
    else{
        mode = 0; //if MSB is 0, use LRU
    }
    if(mode){
        if(Rng(0.03125)){
            lru[set][way] = 0;
        }
        else{
            lru[set][way] = 15; // promote to the MRU position
        }
    }
    else{
        lru[set][way] = 0;
    }
}

// use this function to print out your own stats on every heartbeat 
void PrintStats_Heartbeat()
{

}

// use this function to print out your own stats at the end of simulation
void PrintStats()
{

}
