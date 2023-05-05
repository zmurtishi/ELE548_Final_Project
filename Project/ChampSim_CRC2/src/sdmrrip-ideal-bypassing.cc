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
#define PSEL_MAX 1023
#define TBLSIZE 1<<14
#define CONF_MAX 15
#define LRUBASE 0
#define SAMPLE_SETS 64*NUM_CORE

uint32_t confLocal[LLC_SETS][LLC_WAYS];
uint32_t rrpv[LLC_SETS][LLC_WAYS];
uint32_t privilege[LLC_SETS];
uint64_t conf[NUM_CORE][TBLSIZE];
uint64_t reuse[NUM_CORE][TBLSIZE];
uint32_t bypasses;
uint32_t privilegeEvictions;
uint32_t MRUEntries;
uint32_t psel[NUM_CORE];
uint32_t mode[NUM_CORE];
uint32_t s_sets;
uint32_t bp_mode[NUM_CORE];
uint32_t bp_state[NUM_CORE];
uint64_t dummy;
uint64_t dummy2;
uint32_t last_instruction_count[NUM_CORE];
uint32_t last_cycle_count;

uint32_t accesses[NUM_CORE];

float no_bypassing_ipc[NUM_CORE];
float bypassing_ipc[NUM_CORE];

//int Rng(float p){
//    float rnum = (float)rand()/RAND_MAX;
//    if(rnum < p){
//        return 1;
//    }
//    else{
//	return 0;
//    }
//}

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
    //mode = 1;
    MRUEntries = 0;
    bypasses = 0;
    privilegeEvictions = 0;
    for(int s=0; s<NUM_CORE; s++){
        bypassing_ipc[s] = 0;
        no_bypassing_ipc[s] = 0;
        bp_mode[s] = 0;
        bp_state[s] = 0;
        psel[s] = rand() % PSEL_MAX;
        mode[s] = 0;
        accesses[s] = 0;
        last_instruction_count[s] = 0;
        for(int h=0; h<TBLSIZE; h++){
            conf[s][h] = 7;
            reuse[s][h] = 0;
        }
    }

    //last_instruction_count = 0;
    last_cycle_count = get_cycle_count();

    //accesses = 0;

    //bp_state = 0;
    //bp_mode = 0;
    //psel = PSEL_MAX/2;
    //psel = rand() % PSEL_MAX;
    //table = new uint32_t[TBLSIZE];
    for (int i=0; i<LLC_SETS; i++) {
        for (int j=0; j<LLC_WAYS; j++) {
            rrpv[i][j] = 3;
            confLocal[i][j] = 7;
        }
    }
}


// find replacement victim
// return value should be 0 ~ 15 or 16 (bypass)
uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    uint64_t tblIndex = PC & ((TBLSIZE)-1);
    dummy = PC;
    dummy2 = tblIndex;
    if((conf[cpu][tblIndex] == 0)) {
        if((type != WRITEBACK) && (bp_mode[cpu])){
            if(((set % (s_sets+1)) != 0) && ((set % s_sets) != 0)) { 
                bypasses++;
                return 16;
            }
        }
    }
    while(1){
        for(uint32_t i=0; i<LLC_WAYS; i++){
            if(rrpv[set][i] == RRPV_MAX){
                //if(set % s_sets == 0){
                //   conf[tblIndex] = Sd(conf[tblIndex]);
                //}
                //else if((mode == 1) && (set % (s_sets+1) != 0)){
                //    conf[tblIndex] = Sd(conf[tblIndex]);
                //}
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
    accesses[cpu]++;
    if(way == 16){
        return;
    }
    //dummy = PC;
    if(psel[cpu] > (PSEL_MAX/2)){
        mode[cpu] = 1;
    }
    else{
        mode[cpu] = 0;
    }
    uint64_t tblIndex = PC & ((TBLSIZE)-1);
    uint64_t confidenceValue = conf[cpu][tblIndex];
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
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],1);
	    rrpv[set][way] = 0; //Sd(rrpv[set][way]);
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
            //confLocal[set][way] = Si(confLocal[set][way], CONF_MAX);
        }
        else if(confidenceValue == 0){ 
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX;
            //confLocal[set][way] = conf[tblIndex];
        }
        else if(confidenceValue == 1) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX;
            //confLocal[set][way] = conf[tblIndex];
        }
        else if(confidenceValue == 2) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX-1;
            //confLocal[set][way] = conf[tblIndex];
        }
        else if(confidenceValue == 3) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = RRPV_MAX-1;
            //confLocal[set][way] = conf[tblIndex];
       }
       else if(confidenceValue == 4) {
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Sd(psel[cpu]);
            }
            rrpv[set][way] = 0;
            //confLocal[set][way] = conf[tblIndex];
            MRUEntries++;
       }
    }
    else if((set % (s_sets+1)) == 0){
        if(hit){
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],1);
	    rrpv[set][way] = 0;
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
            //confLocal[set][way] = Si(confLocal[set][way], CONF_MAX);
        }
        else{
            if((type != WRITEBACK)&&(reuse[cpu][tblIndex] != 0)){
                psel[cpu] = Si(psel[cpu], PSEL_MAX);
            }
            rrpv[set][way] = 0;
            //confLocal[set][way] = conf[tblIndex];
        }
    }
    else if(mode[cpu]){
        if(hit){
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],1);
            rrpv[set][way] = 0; //Sd(rrpv[set][way]);
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
            //confLocal[set][way] = Si(confLocal[set][way], CONF_MAX);
        }
        else if(confidenceValue == 0) {
            rrpv[set][way] = RRPV_MAX;
            //confLocal[set][way] = conf[tblIndex];
        }
        else if(confidenceValue == 1) {
            rrpv[set][way] = RRPV_MAX;
            //confLocal[set][way] = conf[tblIndex];
        }
        else if(confidenceValue == 2) {
            rrpv[set][way] = RRPV_MAX-1;
            //confLocal[set][way] = conf[tblIndex];
        }
        else if(confidenceValue == 3) {
            rrpv[set][way] = RRPV_MAX-1;
            //confLocal[set][way] = conf[tblIndex];
       }
       else if(confidenceValue == 4) {
            rrpv[set][way] = 0;
            //confLocal[set][way] = conf[tblIndex];
            MRUEntries++;
       }
    }
    else{
        if(hit){
            reuse[cpu][tblIndex] = Si(reuse[cpu][tblIndex],1);
            rrpv[set][way] = 0;
            conf[cpu][tblIndex] = Si(conf[cpu][tblIndex], CONF_MAX);
            //confLocal[set][way] = Si(confLocal[set][way], CONF_MAX);
        }
        else{
            rrpv[set][way] = 0;
            //confLocal[set][way] = conf[tblIndex];
        }
    }
    //if(conf[tblIndex] == 15)
    //{
    //    privilege[set] = way;
    //}
    switch(bp_state[cpu]){
        case 0:
            if(accesses[cpu] >= 10000){
                no_bypassing_ipc[cpu] = ((float)(get_instr_count(cpu)-last_instruction_count[cpu]))/((float)(get_cycle_count()-last_cycle_count));
                last_cycle_count = get_cycle_count();
                last_instruction_count[cpu] = get_instr_count(cpu);
                bp_state[cpu] = 1;
                bp_mode[cpu] = 1;
                accesses[cpu] = 0;
            }
            break;
        case 1:
            if(accesses[cpu] >= 10000){
                bypassing_ipc[cpu] = ((float)(get_instr_count(cpu)-last_instruction_count[cpu]))/((float)(get_cycle_count()-last_cycle_count));
                last_cycle_count = get_cycle_count();
                last_instruction_count[cpu] = get_instr_count(cpu);
                if(bypassing_ipc[cpu] > no_bypassing_ipc[cpu]){
                    bp_mode[cpu] = 1;
                    bp_state[cpu] = 2;
                }
                else{
                    bp_mode[cpu] = 0;
                    bp_state[cpu] = 3;
                }
                accesses[cpu] = 0;
            }
            break;
        case 2:
            if(accesses[cpu] >= 30000){
                bypassing_ipc[cpu] = ((float)(get_instr_count(cpu)-last_instruction_count[cpu]))/((float)(get_cycle_count()-last_cycle_count));
                last_cycle_count = get_cycle_count();
                last_instruction_count[cpu] = get_instr_count(cpu);
                if(bypassing_ipc[cpu] > no_bypassing_ipc[cpu]){
                    bp_mode[cpu] = 1;
                    bp_state[cpu] = 2;
                }
                else{
                    bp_mode[cpu] = 0;
                    bp_state[cpu] = 3;
                }
                accesses[cpu] = 0;
            }
            break;
        case 3:
            if(accesses[cpu] >= 30000){
                no_bypassing_ipc[cpu] = ((float)(get_instr_count(cpu)-last_instruction_count[cpu]))/((float)(get_cycle_count()-last_cycle_count));
                last_cycle_count = get_cycle_count();
                last_instruction_count[cpu] = get_instr_count(cpu);
                if(bypassing_ipc[cpu] > no_bypassing_ipc[cpu]){
                    bp_mode[cpu] = 1;
                    bp_state[cpu] = 2;
                }
                else{
                    bp_mode[cpu] = 0;
                    bp_state[cpu] = 3;
                }
                accesses[cpu] = 0;
            }
            break;
    }
}

// use this function to print out your own stats on every heartbeat 
void PrintStats_Heartbeat()
{

}

// use this function to print out your own stats at the end of simulation
void PrintStats()
{
    //cout<<"Privilege evictions: "<<privilegeEvictions<<"\n";
    //cout<<"PSEL: "<<psel<<"\n";
    //cout<<"Bypasses: "<<bypasses<<"\n";
    //cout<<"MRUEntries"<<MRUEntries<<"\n";
    //cout<<"Bypassing IPC"<<bypassing_ipc<<"\n";
    //cout<<"No bypassing IPC"<<no_bypassing_ipc<<"\n";
    //for(int i=0; i<TBLSIZE; i++){
    //    cout<<conf[i]<<" ";
    //}
    //cout<<"Dummy: "<<dummy<<"\n";
    //cout<<"Dummy 2: "<<dummy2<<"\n";

    uint32_t sum = 0;
    float total_ipc = 0;
    for(int i=0; i<4; i++){
        sum += get_instr_count(i);
    }
    sum = sum/4;
    total_ipc = (float)sum/get_cycle_count();
    cout<<"Process 1 IPC: "<<(float)get_instr_count(0)/get_cycle_count()<<"\n";
    cout<<"Process 2 IPC: "<<(float)get_instr_count(1)/get_cycle_count()<<"\n";
    cout<<"Process 3 IPC: "<<(float)get_instr_count(2)/get_cycle_count()<<"\n";
    cout<<"Process 4 IPC: "<<(float)get_instr_count(3)/get_cycle_count()<<"\n";
    cout<<"Cumulative IPC: "<<total_ipc<<"\n";


}
