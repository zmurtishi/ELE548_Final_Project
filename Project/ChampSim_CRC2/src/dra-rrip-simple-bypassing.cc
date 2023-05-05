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
#define PSEL_MAX 1024
#define TBLSIZE 1<<14
#define CONF_MAX 15
#define LRUBASE 0
#define SAMPLE_SETS 32

uint32_t confLocal[LLC_SETS][LLC_WAYS];
uint32_t rrpv[LLC_SETS][LLC_WAYS];
uint32_t privilege[LLC_SETS];
uint32_t conf[TBLSIZE];
uint32_t reuse[TBLSIZE];
uint32_t bypasses;
uint32_t privilegeEvictions;
uint32_t MRUEntries;
uint32_t psel[NUM_CORE];
uint32_t mode[NUM_CORE];
uint32_t s_sets;
uint32_t bp_mode;
uint32_t bp_state;
uint32_t numEntries;
uint32_t last_instruction_count;
uint32_t last_cycle_count;
uint32_t hitTbl[TBLSIZE];
uint32_t accesses;
uint32_t bypassBuffer[1024];
float no_bypassing_ipc;
float bypassing_ipc;

int Rng(float p){
    float rnum = (float)rand()/RAND_MAX;
    if(rnum < p){
        return 1;
    }
    else{
	return 0;
    }
}

uint32_t Si(uint32_t n, uint32_t m){
    uint32_t temp;
    if(n < m){
        temp = n + 1;
        return temp;
    }
    return n;
}


uint32_t Sd(uint32_t n){
    uint32_t temp;
    if(n > 0){
        temp = n - 1;
        return temp;
    }
    return n;
}

// initialize replacement state
void InitReplacementState()
{
    s_sets = LLC_SETS/SAMPLE_SETS;
    cout << "Initialize SRRIP replacement state" << endl;
    MRUEntries = 0;
    bypasses = 0;
    privilegeEvictions = 0;

    bypassing_ipc = 0;
    no_bypassing_ipc = 0;

    last_instruction_count = 0;
    last_cycle_count = 0;

    accesses = 0;
    numEntries = 0;
    bp_state = 0;
    bp_mode = 0;
    for(int e=0; e<1024; e++){
        bypassBuffer[e] = TBLSIZE;
    }
    //psel = PSEL_MAX/2;
    for(int c=0; c<NUM_CORE; c++){
        psel[c] = rand() % PSEL_MAX;
        mode[c] = 0;
    }

    for (int i=0; i<LLC_SETS; i++) {
        for (int j=0; j<LLC_WAYS; j++) {
            rrpv[i][j] = 3;
            confLocal[i][j] = 7;
        }
    }
    for(int k=0; k<TBLSIZE; k++){
        conf[k] = 7;
        reuse[k] = 0;
        hitTbl[k] = 0;
    }
    for(int l=0; l<LLC_SETS; l++){
        privilege[l] = 16;
    }
}

uint32_t searchBuffer(uint32_t arr[], uint32_t val){
    for(int i=0; i<1024; i++){
        if(arr[i] == val){
            return 1;
        }
    }
    return 0;
}

// find replacement victim
// return value should be 0 ~ 15 or 16 (bypass)
uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    uint32_t tblIndex = PC & ((TBLSIZE)-1);
    if((conf[tblIndex] == 0)) {
        if((type != WRITEBACK)){
            if(((set % (s_sets+1)) != 0) && ((set % s_sets) != 0)) { 
                bypasses++;
                if(searchBuffer(bypassBuffer, tblIndex)){
                    hitTbl[tblIndex]++;
                }
                else{
                    uint32_t tempIndex = numEntries % 1024;
                    bypassBuffer[tempIndex] = tblIndex;
                    numEntries++;
                }
                return 16;
            }
        }
    }
    if((conf[tblIndex] == 1)) {
        if((type != WRITEBACK) && Rng(0.7)){
            if(((set % (s_sets+1)) != 0) && ((set % s_sets) != 0)) { 
                bypasses++;
                return 16;
            }
        }
    }
    //uint32_t tblIndex = PC % TBLSIZE;
    while(1){
        for(uint32_t i=0; i<LLC_WAYS; i++){
            if((rrpv[set][i] == RRPV_MAX) && (i != privilege[set])){
                conf[tblIndex] = Sd(conf[tblIndex]);
                return i;
             }
             if(i == privilege[set]){
                   privilegeEvictions++;
             }
        }
        for(uint32_t j=0; j<LLC_WAYS; j++){
            rrpv[set][j] = Si(rrpv[set][j], RRPV_MAX);
        }
    }
}

// called on every cache hit and cache fill
void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    accesses++;
    if(way == 16){
        return;
    }
    if(psel[cpu] > (PSEL_MAX/2)){
        mode[cpu] = 1;
    }
    else{
        mode[cpu] = 0;
    }
    uint32_t tblIndex = PC & ((TBLSIZE)-1);
    //uint32_t tblIndex = PC % TBLSIZE;
    uint32_t confidenceValue = conf[tblIndex];
    if(type == WRITEBACK){
        confidenceValue = 0;
    }
    if(confidenceValue == 0){
        confidenceValue = 0;
    }
    else if(confidenceValue < 5) {
        confidenceValue = 1;
    }
    else if(confidenceValue == 15){
        confidenceValue = 4;
    }
    else if(confidenceValue > 9){
        confidenceValue = 2;
    }
    else{
        confidenceValue = 3;
    }
    if((set % s_sets) == 0){
        if(hit){
            reuse[tblIndex] = Si(reuse[tblIndex],1);
	    rrpv[set][way] = 0; //Sd(rrpv[set][way]);
            conf[tblIndex] = Si(conf[tblIndex], CONF_MAX);
        }
        else if(confidenceValue == 0){ 
            if((type != WRITEBACK)&&(reuse[tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX;
        }
        else if(confidenceValue == 1) {
            if((type != WRITEBACK)&&(reuse[tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX;
        }
        else if(confidenceValue == 2) {
            if((type != WRITEBACK)&&(reuse[tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX-2;
        }
        else if(confidenceValue == 3) {
            if((type != WRITEBACK)&&(reuse[tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX-1;
       }
       else if(confidenceValue == 4) {
            if((type != WRITEBACK)&&(reuse[tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = 0;
            MRUEntries++;
       }
    }
    else if((set % (s_sets+1)) == 0){
        if(hit){
            reuse[tblIndex] = Si(reuse[tblIndex],1);
	    rrpv[set][way] = 0;
            conf[tblIndex] = Si(conf[tblIndex], CONF_MAX);
        }
        else{
            if((type != WRITEBACK)&&(reuse[tblIndex] != 0)){
                psel[cpu] = Si(psel[cpu], PSEL_MAX);
            }
            rrpv[set][way] = 0;
        }
    }
    else if(mode[cpu]){
        if(hit){
            reuse[tblIndex] = Si(reuse[tblIndex],1);
            rrpv[set][way] = 0;
            conf[tblIndex] = Si(conf[tblIndex], CONF_MAX);
        }
        else if(confidenceValue == 0) {
            rrpv[set][way] = RRPV_MAX;
        }
        else if(confidenceValue == 1) {
            rrpv[set][way] = RRPV_MAX;
        }
        else if(confidenceValue == 2) {
            rrpv[set][way] = RRPV_MAX-2;
        }
        else if(confidenceValue == 3) {
            rrpv[set][way] = RRPV_MAX-1;
       }
       else if(confidenceValue == 4) {
            rrpv[set][way] = 0;
            MRUEntries++;
       }
    }
    else{
        if(hit){
            reuse[tblIndex] = Si(reuse[tblIndex],1);
            rrpv[set][way] = 0;
            conf[tblIndex] = Si(conf[tblIndex], CONF_MAX);
        }
        else{
            rrpv[set][way] = 0;
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
    cout<<"Privilege evictions: "<<privilegeEvictions<<"\n";
    cout<<"PSEL: "<<psel<<"\n";
    cout<<"Bypasses: "<<bypasses<<"\n";
    cout<<"MRUEntries"<<MRUEntries<<"\n";
    cout<<"Bypassing IPC"<<bypassing_ipc<<"\n";
    cout<<"No bypassing IPC"<<no_bypassing_ipc<<"\n";

    cout<<"Hit table: \n";
    for(int i=0; i<1024; i++){
        if(bypassBuffer[i]<TBLSIZE){
            cout<<bypassBuffer[i]<<" "<<hitTbl<<" "<<hitTbl[bypassBuffer[i]]<<"\n";
        }
    }
}

