// EtherCat demo program (Based on the Etherlab "user" example).
//
// This demo assumes the following ethercat slaves (in this order):
// 0  0:0  Michael
//
//



#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/****************************************************************************/

#include "ecrt.h"
#include "slaves.h" // generated with the "ethercat cstruct" command
#include "Michael.h" // ucontroller variables definition

/****************************************************************************/

// Controls how often the cyclic_task() routine is called (in usec)
#define FREQUENCY 1000 //1kHz

// If not 0, give this process a higher priority (requires root priv)
#define PRIORITY 0

// Optional features
#define SDO_ACCESS      0

/****************************************************************************/

// EtherCAT master
static ec_master_t *master = NULL;
static ec_master_state_t master_state = {};

// EtherCAT domain
static ec_domain_t *domain1 = NULL;
static ec_domain_state_t domain1_state = {};

// Timer
static unsigned int sig_alarms = 0;
static unsigned int user_alarms = 0;

/****************************************************************************/

// process data (PD)
static uint8_t *domain1_pd = NULL;

// Device positions
#define MichaelPos  0, 0

// This demo application is hard wired to use the following devices (in this order). 
// See the generated slaves.h file.

// Define a struct for each slave to hold values read or written
static MichaelStruct michael; // only slave

const static ec_pdo_entry_reg_t domain1_regs[] = {
    // Slave 1: Michael
    {MichaelPos, Michael, 0x7000, 0x01, &michael.offset_pos_ref, &michael.bit_pos_pos_ref},
    {MichaelPos, Michael, 0x7000, 0x02, &michael.offset_vel_ref, &michael.bit_pos_vel_ref},
    {MichaelPos, Michael, 0x7000, 0x03, &michael.offset_tor_ref, &michael.bit_pos_tor_ref},
    {MichaelPos, Michael, 0x7000, 0x04, &michael.offset_ImpPosPGain, &michael.bit_pos_ImpPosPGain},
    {MichaelPos, Michael, 0x7000, 0x05, &michael.offset_ImpPosDGain, &michael.bit_pos_ImpPosDGain},
    {MichaelPos, Michael, 0x7000, 0x06, &michael.offset_ImpTorPGain, &michael.bit_pos_ImpTorPGain},
    {MichaelPos, Michael, 0x7000, 0x07, &michael.offset_ImpTorDGain, &michael.bit_pos_ImpTorDGain},
    {MichaelPos, Michael, 0x7000, 0x08, &michael.offset_Gain4, &michael.bit_pos_Gain4},
    {MichaelPos, Michael, 0x7000, 0x09, &michael.offset_fault_ack, &michael.bit_pos_fault_ack},
    {MichaelPos, Michael, 0x7000, 0x0a, &michael.offset_ts, &michael.bit_pos_ts},
    {MichaelPos, Michael, 0x7000, 0x0b, &michael.offset_op_idx_aux, &michael.bit_pos_op_idx_aux},
    {MichaelPos, Michael, 0x7000, 0x0c, &michael.offset_aux1, &michael.bit_pos_aux1},

    {MichaelPos, Michael, 0x6000, 0x01, &michael.offset_link_pos, &michael.bit_pos_link_pos},
    {MichaelPos, Michael, 0x6000, 0x02, &michael.offset_motor_pos, &michael.bit_pos_motor_pos},
    {MichaelPos, Michael, 0x6000, 0x03, &michael.offset_link_vel, &michael.bit_pos_link_vel},
    {MichaelPos, Michael, 0x6000, 0x04, &michael.offset_motor_vel, &michael.bit_pos_motor_vel},
    {MichaelPos, Michael, 0x6000, 0x05, &michael.offset_tor, &michael.bit_pos_tor},
    {MichaelPos, Michael, 0x6000, 0x06, &michael.offset_temperature, &michael.bit_pos_temperature},
    {MichaelPos, Michael, 0x6000, 0x07, &michael.offset_fault, &michael.bit_pos_fault},
    {MichaelPos, Michael, 0x6000, 0x08, &michael.offset_rtt, &michael.bit_pos_rtt},
    {MichaelPos, Michael, 0x6000, 0x09, &michael.offset_op_idx_ack, &michael.bit_pos_op_idx_ack},
    {MichaelPos, Michael, 0x6000, 0x0a, &michael.offset_aux2, &michael.bit_pos_aux2},
    {}
};

static unsigned int counter = 0;
static unsigned int blink = 0;


#if SDO_ACCESS
static ec_sdo_request_t *sdo;
#endif

/*****************************************************************************/

static void check_domain1_state(void)
{
    
    ec_domain_state_t ds;

    ecrt_domain_state(domain1, &ds);

    if (ds.working_counter != domain1_state.working_counter)
        printf("Domain1: WC %u.\n", ds.working_counter);
    if (ds.wc_state != domain1_state.wc_state)
        printf("Domain1: State %u.\n", ds.wc_state);

    domain1_state = ds;
}

/*****************************************************************************/

static void check_master_state(void)
{
    ec_master_state_t ms;

    ecrt_master_state(master, &ms);

    if (ms.slaves_responding != master_state.slaves_responding)
        printf("%u slave(s).\n", ms.slaves_responding);
    if (ms.al_states != master_state.al_states)
        printf("AL states: 0x%02X.\n", ms.al_states);
    if (ms.link_up != master_state.link_up)
        printf("Link is %s.\n", ms.link_up ? "up" : "down");

    master_state = ms;
}

/*****************************************************************************/
// 
static void check_slave_config_states(char* name, ec_slave_config_t* config, ec_slave_config_state_t* state)
{
    ec_slave_config_state_t s;

    ecrt_slave_config_state(config, &s);

    if (s.al_state != state->al_state)
        printf("%s: State 0x%02X.\n", name, s.al_state);
    if (s.online != state->online)
        printf("%s: %s.\n", name, s.online ? "online" : "offline");
    if (s.operational != state->operational)
        printf("%s: %soperational.\n", name, s.operational ? "" : "Not ");

    *state = s;
}

/*****************************************************************************/

#if SDO_ACCESS
static void read_sdo(void)
{
    switch (ecrt_sdo_request_state(sdo)) {
        case EC_REQUEST_UNUSED: // request was not used yet
            ecrt_sdo_request_read(sdo); // trigger first read
            break;
        case EC_REQUEST_BUSY:
            fprintf(stderr, "Still busy...\n");
            break;
        case EC_REQUEST_SUCCESS:
            fprintf(stderr, "SDO value: 0x%04X\n",
                    EC_READ_U16(ecrt_sdo_request_data(sdo)));
            ecrt_sdo_request_read(sdo); // trigger next read
            break;
        case EC_REQUEST_ERROR:
            fprintf(stderr, "Failed to read SDO!\n");
            ecrt_sdo_request_read(sdo); // retry reading
            break;
    }
}
#endif


/****************************************************************************/
// Do the write for Michael: write data without meaning
// (Note: Setting a tristate bit to 1 turns the devices LED yellow and disables the output.
static void write_process_data_Michael() {
    EC_WRITE_BIT(domain1_pd + michael.offset_pos_ref, michael.bit_pos_pos_ref, 0x00);
}

/****************************************************************************/
static void write_process_data() {
    write_process_data_Michael();
}

static void read_process_data() {
    
    printf(" %u\n ",
            //EC_READ_U32(domain1_pd + michael.offset_pos_ref), 
            //EC_READ_U16(domain1_pd + michael.offset_vel_ref), 
            //EC_READ_U16(domain1_pd + michael.offset_tor_ref),
            //EC_READ_U16(domain1_pd + michael.offset_ImpPosPGain),
            //EC_READ_U16(domain1_pd + michael.offset_ImpPosDGain),
            //EC_READ_U16(domain1_pd + michael.offset_ImpTorPGain),
            //EC_READ_U16(domain1_pd + michael.offset_ImpTorDGain),
            //EC_READ_U16(domain1_pd + michael.offset_Gain4),
            //EC_READ_U16(domain1_pd + michael.offset_fault_ack),
            //EC_READ_U16(domain1_pd + michael.offset_ts),
            //EC_READ_U16(domain1_pd + michael.offset_op_idx_aux),
            //EC_READ_U32(domain1_pd + michael.offset_aux1),              
            EC_READ_U32(domain1_pd + michael.offset_link_pos));          //12
            //EC_READ_S32(domain1_pd + michael.offset_motor_pos),         //13
            //EC_READ_S16(domain1_pd + michael.offset_link_vel));         //14
            //EC_READ_U16(domain1_pd + michael.offset_motor_vel),         
            //EC_READ_U32(domain1_pd + michael.offset_tor),               //15
            //EC_READ_U16(domain1_pd + michael.offset_temperature),       //16
            //EC_READ_U16(domain1_pd + michael.offset_fault),
            //EC_READ_U16(domain1_pd + michael.offset_rtt),
            //EC_READ_U16(domain1_pd + michael.offset_op_idx_ack),
            ////EC_READ_U16(domain1_pd + michael.offset_aux2));
       
    //printf("Slave1 Out1: value %u\n", EC_READ_U16(domain1_pd + michael.offset_temperature)); 
}

/****************************************************************************/
// ONCE THE MASTER IS ACTIVATED, THE APP IS IN CHARGE OF EXCHANGING DATA THROUGH
// EXPLICIT CALLS TO THE ECRT LIBRARY (DONE IN THE IDLE STATE BY THE MASTER)
static void cyclic_task()
{
    int i;

    // receive process data
    ecrt_master_receive(master);  // RECEIVE A FRAME
    ecrt_domain_process(domain1); // DETERMINE THE DATAGRAM STATES

    // check process data state (optional)
    check_domain1_state();

    if (counter) {
        counter--;
    } else { // do this at 10 Hz
        counter = FREQUENCY;

        // check for master state (optional)
        check_master_state();

        // check for islave configuration state(s) (optional)
        check_slave_config_states("Slave1", michael.config, &michael.config_state);

#if SDO_ACCESS
        // read process data SDO
        read_sdo();
#endif

    }

#if 0
    // read process data
    printf("Slave1 Out1: %u\n",
            EC_READ_BIT(domain1_pd + michael.offset_pos_ref, michael.bit_pos_pos_ref));
#endif

    // write process data
    //write_process_data();

    // read process data
    read_process_data();

    // send process data
    ecrt_domain_queue(domain1); // MARK THE DOMAIN DATA AS READY FOR EXCHANGE
    ecrt_master_send(master);   // SEND ALL QUEUED DATAGRAMS
}

/****************************************************************************/

static void signal_handler(int signum) {
    switch (signum) {
        case SIGALRM:
            sig_alarms++;
            break;
    }
}

/****************************************************************************/
// Sets the timer for the cyclic task.
// Returns non-zero on error.
static int set_timer() {
    struct sigaction sa;
    struct itimerval tv;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, 0)) {
        fprintf(stderr, "Failed to install signal handler!\n");
        return -1;
    }

    printf("Starting timer...\n");
    tv.it_interval.tv_sec = 0;
    tv.it_interval.tv_usec = 1000000 / FREQUENCY;
    tv.it_value.tv_sec = 0;
    tv.it_value.tv_usec = 1000;

    if (setitimer(ITIMER_REAL, &tv, NULL)) {
        fprintf(stderr, "Failed to start timer: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}

/****************************************************************************/
// Configures the PDO given the address of the slave's config pointer, syncs (from slaves.h),
// the slave's position and vendor info.
// Returns non-zero on error.
static int configure_pdo(
    ec_slave_config_t** config, // output param
    ec_sync_info_t* syncs, 
    uint16_t alias, 
    uint16_t position, 
    uint32_t vendor_id, 
    uint32_t product_code) {

    if (!(*config = ecrt_master_slave_config(master, alias, position, vendor_id, product_code))) {
        fprintf(stderr, "Failed to get slave configuration.\n");
        return -1;
    }

    if (ecrt_slave_config_pdos(*config, EC_END, syncs)) {
        fprintf(stderr, "Failed to configure PDOs.\n");
        return -1;
    }
    return 0;
}

/****************************************************************************/
int main(int argc, char **argv)
{
    ec_slave_config_t *sc;
    
    // FIRST, REQUEST A MASTER INSTANCE
    master = ecrt_request_master(0);
    if (!master)
        return -1;

    // THEN, CREATE A DOMAIN
    domain1 = ecrt_master_create_domain(master);
    if (!domain1)
        return -1;

#if SDO_ACCESS
    fprintf(stderr, "Creating SDO requests...\n");
    if (!(sdo = ecrt_slave_config_create_sdo_request(el2202.config, 
						     slave_1_pdo_entries[0].index, 
						     slave_1_pdo_entries[0].subindex, 
						     slave_1_pdo_entries[0].bitlength))) {
        fprintf(stderr, "Failed to create SDO request.\n");
        return -1;
    }
    ecrt_sdo_request_timeout(sdo, 500); // ms
#endif

    printf("Configuring PDOs...\n");
    if (configure_pdo(&michael.config, slave_0_syncs, MichaelPos, Michael)) return -1;

    if (ecrt_domain_reg_pdo_entry_list(domain1, domain1_regs)) {
        fprintf(stderr, "PDO entry registration failed!\n");
        return -1;
    }

    // ACTIVATE THE MASTER. DO NOT APPLY ANY CONFIGURATION AFTER THIS, IT WON'T WORK
    printf("Activating master...\n");
    if (ecrt_master_activate(master))
        return -1;

    // INITIALIZE THE PROCESS DOMAIN MEMORY (FOR USER-SPACE APPS)
    if (!(domain1_pd = ecrt_domain_data(domain1))) {
        return -1;

    }

#if PRIORITY
    pid_t pid = getpid();
    if (setpriority(PRIO_PROCESS, pid, -19))
        fprintf(stderr, "Warning: Failed to set priority: %s\n", strerror(errno));
#endif

    int timer_status = set_timer();
    if (timer_status) return timer_status;

    printf("Started.\n");
    while (1) {
        pause();

#if 0
        struct timeval t;
        gettimeofday(&t, NULL);
        printf("%u.%06u\n", t.tv_sec, t.tv_usec);
#endif
        
        while (sig_alarms != user_alarms) {
            cyclic_task();
            user_alarms++;
        }
    }

    return 0;
}

/****************************************************************************/
