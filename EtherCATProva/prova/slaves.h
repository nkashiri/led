// generated with the "ethercat cstruct" command

/* Master 0, Slave 0, "SM_Motor"
 * Vendor ID:       0x00000298
 * Product code:    0x00000012
 * Revision number: 0x00000001
 */

ec_pdo_entry_info_t slave_0_pdo_entries[] = {
    {0x7000, 0x01, 32}, /* pos_ref */
    {0x7000, 0x02, 16}, /* vel_ref */
    {0x7000, 0x03, 16}, /* tor_ref */
    {0x7000, 0x04, 16}, /* ImpPosPGain */
    {0x7000, 0x05, 16}, /* ImpPosDGain */
    {0x7000, 0x06, 16}, /* ImpTorPGain */
    {0x7000, 0x07, 16}, /* ImpTorDGain */
    {0x7000, 0x08, 16}, /* Gain4 */
    {0x7000, 0x09, 16}, /* fault ack */
    {0x7000, 0x0a, 16}, /* ts */
    {0x7000, 0x0b, 16}, /* op_idx_aux */
    {0x7000, 0x0c, 32}, /* aux */
    {0x6000, 0x01, 32}, /* link pos */
    {0x6000, 0x02, 32}, /* motor pos */
    {0x6000, 0x03, 16}, /* link vel */
    {0x6000, 0x04, 16}, /* motor vel */
    {0x6000, 0x05, 32}, /* tor */
    {0x6000, 0x06, 16}, /* temperature */
    {0x6000, 0x07, 16}, /* fault */
    {0x6000, 0x08, 16}, /* rtt */
    {0x6000, 0x09, 16}, /* op_idx_ack */
    {0x6000, 0x0a, 32}, /* aux */
};

ec_pdo_info_t slave_0_pdos[] = {
    {0x1600, 12, slave_0_pdo_entries + 0}, /* Receive PDO Mapping */
    {0x1a00, 10, slave_0_pdo_entries + 12}, /* Transmit PDO Mapping */
};

ec_sync_info_t slave_0_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, slave_0_pdos + 0, EC_WD_DISABLE},
    {3, EC_DIR_INPUT, 1, slave_0_pdos + 1, EC_WD_DISABLE},
    {0xff}
};
