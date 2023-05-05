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
#define CONF_MAX 31
#define LRUBASE 0
#define SAMPLE_SETS 64
#define THRESHOLD 0.02
#define BSIZE 16
#define REUSE_MAX 1
#define HI_REUSE 10000000
#define INTERVAL 4096
#define LCA 10000
#define LCR 200

uint32_t confLocal[LLC_SETS][LLC_WAYS];
uint32_t rrpv[LLC_SETS][LLC_WAYS];
uint32_t privilege[LLC_SETS];
uint32_t conf[NUM_CORE][TBLSIZE];
uint32_t reuse[NUM_CORE][TBLSIZE];
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
//uint32_t hitTbl[TBLSIZE];
uint32_t accesses[NUM_CORE];
uint32_t bypassBuffer[BSIZE];
float no_bypassing_ipc;
float bypassing_ipc;
uint32_t failed_bypasses;
uint32_t high_reuse[NUM_CORE];
uint32_t low_confidence_reuses[NUM_CORE];
uint32_t low_confidence_accesses[NUM_CORE];
uint32_t low_confidence_misses[NUM_CORE];
uint32_t bypassState[NUM_CORE];

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
    for(int y=0; y<NUM_CORE; y++){
        accesses[y]=0;
        high_reuse[y] = 0;
        low_confidence_reuses[y] = 0;
        low_confidence_accesses[y] = 0;
        low_confidence_misses[y] = 0;
        bypassState[y] = 0;
    }

    //bypassState = 0;
    bypassing_ipc = 0;
    no_bypassing_ipc = 0;

    last_instruction_count = 0;
    last_cycle_count = 0;

    //accesses = 0;
    numEntries = 0;
    bp_state = 0;
    bp_mode = 0;
    failed_bypasses = 0;
    for(int e=0; e<BSIZE; e++){
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
       for(int v = 0; v<NUM_CORE; v++){
           conf[v][k] = CONF_MAX/2;
           reuse[v][k] = 0;
           //hitTbl[v][k] = 0;
       }
    }
    for(int l=0; l<LLC_SETS; l++){
        privilege[l] = 16;
    }
}

uint32_t searchBuffer(uint32_t val){
    for(int i=0; i<BSIZE; i++){
        if(bypassBuffer[i] == val){
            return 1;
        }
    }
    return 0;
}

//void addEntry(uint32_t value){
//    if(numEntries == BSIZE){
//        uint32_t minValue = hitTbl[bypassBuffer[0]];
//        uint32_t minIndex = 0;
//        for(int i=0; i<BSIZE; i++){
//            if(hitTbl[bypassBuffer[i]] < minValue){
//                  minIndex = i;
//                  //minValue = hitTbl[bypassBuffer[i]];
//           }
//        }
//        bypassBuffer[minIndex] = value;
//    }
//    else{
//        bypassBuffer[numEntries] = value;
//        numEntries++;
//    }
//
//}

// find replacement victim
// return value should be 0 ~ 15 or 16 (bypass)
uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    uint32_t tblIndex = PC & ((TBLSIZE)-1);

    switch(bypassState[cpu]){
        case 0:
            if(((low_confidence_accesses[cpu] % 10000)==0)&&(low_confidence_accesses[cpu]>0)){
                if((float)low_confidence_reuses[cpu]/low_confidence_misses[cpu] < THRESHOLD){
                    high_reuse[cpu] = 1;
                    bypassState[cpu] = 1;
                    accesses[cpu] = 0;
                }
                else{
                    high_reuse[cpu] = 0;
                }
            }
            break;
        case 1:
            if(((accesses[cpu] % 100000)==0)&&(accesses[cpu]>0)){
                bypassState[cpu] = 0;
                high_reuse[cpu] = 0;
                low_confidence_reuses[cpu] = 0;
                low_confidence_misses[cpu] = 0;
                low_confidence_accesses[cpu] = 0;
            }
            break;
    }
    if(type != WRITEBACK){
        if((conf[cpu][tblIndex] == 0)&&(high_reuse[cpu])){
            if((((set % (s_sets+1)) != 0) && ((set % s_sets) != 0))){
                return 16;
            }
        }
       // if(searchBuffer(tblIndex)){
       //     if((hitTbl[tblIndex] > THRESHOLD)){
       //         if((((set % (s_sets+1)) != 0) && ((set % s_sets) != 0))){
       //             hitTbl[tblIndex] = 0;
       //             return 16;
       //         }
       //     }
       // }
    }
    while(1){
        for(uint32_t i=0; i<LLC_WAYS; i++){
            if(rrpv[set][i] == RRPV_MAX){
                conf[cpu][tblIndex] = Sd(conf[cpu][tblIndex]);
                return i;
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
    uint32_t tblIndex = PC & ((TBLSIZE)-1);
    if(conf[cpu][tblIndex]==0){
        accesses[cpu]++;
    }
    if(way == 16){
        return;
    }
    //uint32_t tblIndex = PC & ((TBLSIZE)-1);
    if(conf[cpu][tblIndex]==0){
        if(hit == 1){
            low_confidence_reuses[cpu]++;
        }
        else{
            low_confidence_misses[cpu]++;
        }
        low_confidence_accesses[cpu]++;
    }
    if(psel[cpu] > (PSEL_MAX/2)){
        mode[cpu] = 1;
    }
    else{
        mode[cpu] = 0;
    }
    uint32_t confidenceValue = conf[cpu][tblIndex];
    if(type == WRITEBACK){
        confidenceValue = 0;
    }
    if(confidenceValue == 0){
        confidenceValue = 0;
    }
    else if(confidenceValue < CONF_MAX/2-3) {
        confidenceValue = 1;
    }
    else if(confidenceValue == CONF_MAX){
        confidenceValue = 4;
    }
    else if(confidenceValue > CONF_MAX/2+3){
        confidenceValue = 2;
    }
    else{
        confidenceValue = 3;
    }
    if((set % s_sets) == 0){
        if(hit){
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],REUSE_MAX);
	    rrpv[set][way] = 0; //Sd(rrpv[set][way]);
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
        }
        else if(confidenceValue == 0){ 
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX;
        }
        else if(confidenceValue == 1) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX;
        }
        else if(confidenceValue == 2) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX-2;
        }
        else if(confidenceValue == 3) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX-1;
       }
       else if(confidenceValue == 4) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = 0;
            MRUEntries++;
       }
    }
    else if((set % (s_sets+1)) == 0){
        if(hit){
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],REUSE_MAX);
	    rrpv[set][way] = 0;
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
        }
        else{
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Si(psel[cpu], PSEL_MAX);
            }
            rrpv[set][way] = 0;
        }
    }
    else if(mode[cpu]){
        if(hit){
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],REUSE_MAX);
            rrpv[set][way] = 0;
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
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
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],REUSE_MAX);
            rrpv[set][way] = 0;
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
        }
        else{
            rrpv[set][way] = 0;
        }
    }
}

// use this function to print out your own stats on every heartbeat 
void PrintStats_Heartbeat()
{
    //cout<<"Low-conf ratio heartbeat: "<<(double)low_confidence_reuses/low_confidence_misses<<" ";
}

// use this function to print out your own stats at the end of simulation
void PrintStats()
{
    //cout<<"Privilege evictions: "<<privilegeEvictions<<"\n";
    //cout<<"PSEL: "<<psel[0]<<"\n";
    //cout<<"Bypasses: "<<bypasses<<"\n";
    //cout<<"MRUEntries"<<MRUEntries<<"\n";
    //cout<<"Bypassing IPC"<<bypassing_ipc<<"\n";
    //cout<<"No bypassing IPC"<<no_bypassing_ipc<<"\n";

    //cout<<"Hit table: \n";
    //for(int i=0; i<BSIZE; i++){
        //if(bypassBuffer[i]<TBLSIZE){
            //cout<<bypassBuffer[i]<<" "<<hitTbl[bypassBuffer[i]]<<"\n";
    //    }
    //}
    //cout<<"Low-conf reuses: "<<low_confidence_reuses;
    cout<<"PSEL 0"<<psel[0]<<"\n";
    cout<<"PSEL 1"<<psel[1]<<"\n";
    cout<<"PSEL 2"<<psel[2]<<"\n";
    cout<<"PSEL 3"<<psel[3]<<"\n";
    uint32_t sum = 0;
    float total_ipc = 0;
    for(int i=0; i<4; i++){
        sum += get_instr_count(i);
    }
    sum = sum/4;
    total_ipc = (float)sum/get_cycle_count();
    cout<<"Process 1 IPC: "<<(float)get_instr_count(0)/get_cycle_count();
    cout<<"Process 2 IPC: "<<(float)get_instr_count(1)/get_cycle_count();
    cout<<"Process 3 IPC: "<<(float)get_instr_count(2)/get_cycle_count();
    cout<<"Process 4 IPC: "<<(float)get_instr_count(3)/get_cycle_count();
    cout<<"Cumulative IPC: "<<total_ipc<<"\n";
}

