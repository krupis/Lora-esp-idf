
#ifndef __LORA_H__
#define __LORA_H__

void lora_reset(void);
void lora_explicit_header_mode(void);
void lora_implicit_header_mode(int size);
void lora_idle(void);
void lora_sleep(void); 
void lora_receive(void);
void lora_set_tx_power(int level);
void lora_set_frequency(long frequency);
void lora_set_spreading_factor(int sf);
void lora_set_bandwidth(long sbw);
void lora_set_coding_rate(int denominator);
void lora_set_preamble_length(long length);
void lora_set_sync_word(int sw);
void lora_enable_crc(void);
void lora_disable_crc(void);
int lora_init(void);
void lora_send_packet(uint8_t *buf, int size);
int lora_receive_packet(uint8_t *buf, int size);
int lora_received(void);
int lora_packet_rssi(void);
float lora_packet_snr(void);
void lora_close(void);
int lora_initialized(void);
void lora_dump_registers(void);
int lora_read_reg(int reg);
void lora_write_reg(int reg, int val);

#ifdef CONFIG_IDF_TARGET_ESP32S3
    #define LORA_CS_PIN         21
    #define LORA_RST_PIN        20
    #define LORA_MOSI_PIN       45
    #define LORA_MISOx_PIN      47
    #define LORA_SCK_PIN        48
#endif
#ifdef CONFIG_IDF_TARGET_ESP32
    #define LORA_CS_PIN         21
    #define LORA_RST_PIN        13
    #define LORA_MOSI_PIN       23
    #define LORA_MISOx_PIN      19
    #define LORA_SCK_PIN        22
#endif



#endif
