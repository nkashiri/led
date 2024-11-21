// Data related to testing Michael (ucontroller)
// to define variables, look at slaves.h
// Michael ucontroller for testing etherCAT
#define Michael 0x00000298, 0x00000012

// Holds values written to or read from the device
/// @brief 
typedef struct {
    // Stores PDO entry's (byte-)offset in the process data. 
    unsigned int offset_pos_ref;
    unsigned int offset_vel_ref;
    unsigned int offset_tor_ref; 
    unsigned int offset_ImpPosPGain;
    unsigned int offset_ImpPosDGain;
    unsigned int offset_ImpTorPGain;
    unsigned int offset_ImpTorDGain;
    unsigned int offset_Gain4;
    unsigned int offset_fault_ack;
    unsigned int offset_ts;
    unsigned int offset_op_idx_aux;
    unsigned int offset_aux1;
    unsigned int offset_link_pos;
    unsigned int offset_motor_pos;
    unsigned int offset_link_vel;
    unsigned int offset_motor_vel;
    unsigned int offset_tor;
    unsigned int offset_temperature;   
    unsigned int offset_fault;
    unsigned int offset_rtt;
    unsigned int offset_op_idx_ack;
    unsigned int offset_aux2;


    // Store a bit position (0-7) within the above offset
    unsigned int bit_pos_pos_ref;
    unsigned int bit_pos_vel_ref;
    unsigned int bit_pos_tor_ref; 
    unsigned int bit_pos_ImpPosPGain;
    unsigned int bit_pos_ImpPosDGain;
    unsigned int bit_pos_ImpTorPGain;
    unsigned int bit_pos_ImpTorDGain;
    unsigned int bit_pos_Gain4;
    unsigned int bit_pos_fault_ack;
    unsigned int bit_pos_ts;
    unsigned int bit_pos_op_idx_aux;
    unsigned int bit_pos_aux1;
    unsigned int bit_pos_link_pos;
    unsigned int bit_pos_motor_pos;
    unsigned int bit_pos_link_vel;
    unsigned int bit_pos_motor_vel;
    unsigned int bit_pos_tor;
    unsigned int bit_pos_temperature;   
    unsigned int bit_pos_fault;
    unsigned int bit_pos_rtt;
    unsigned int bit_pos_op_idx_ack;
    unsigned int bit_pos_aux2;
    
    // Stores all the 1 bit values
    ec_slave_config_t* config;

    // Stores the configuration states of out1, out2
    ec_slave_config_state_t config_state;

} MichaelStruct;
