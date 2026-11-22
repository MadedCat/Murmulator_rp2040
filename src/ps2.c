#include "pico.h"
#include "ps2.h"
#include "hardware/dma.h"
#include "kb_u_codes.h"
#include "string.h"
#include "globals.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"
#include "hardware/structs/dma.h"
#include "hardware/irq.h"

//#define FAST_FUNC __not_in_flash_func

#if !PICO_NO_HARDWARE
	#include "hardware/pio.h"
#endif


#define LEN_PS2_BUF (100)
static uint32_t PS_2_BUS[LEN_PS2_BUF];

static uint32_t inx_rd_buf=0;

static PIO pio_ps2=PIO_PS2;
static int sm_ps2=SM_PS2;

/*
volatile uint8_t _tx_ready;         // TX status for type of send contains
            / _HANDSHAKE 0x80 = handshaking command (ECHO/RESEND)
               _COMMAND   0x01 = other command processing /

volatile uint8_t _bitcount;          // Main state variable and bit count for interrupts
volatile uint8_t _shiftdata;
volatile uint8_t _parity;
volatile uint8_t _last_sent;        // last byte if resend requested
volatile uint8_t _now_send;         // immediate byte to send
volatile uint8_t _response_count;   // bytes expected in reply to next TX

// Private Variables
volatile uint8_t _ps2mode;          / _ps2mode contains
    _PS2_BUSY      bit 7 = busy until all expected bytes RX/TX
    _TX_MODE       bit 6 = direction 1 = TX, 0 = RX (default)
    _BREAK_KEY     bit 5 = break code detected
    _WAIT_RESPONSE bit 4 = expecting data response
    _E0_MODE       bit 3 = in E0 mode
    _E1_MODE       bit 2 = in E1 mode
    _LAST_VALID    bit 1 = last sent valid in case we receive resend
                           and not sent anything /
*/

static uint16_t pio_program_PS2_instructions[] = {
            //     .wrap                
    0xe02a, //  0: set    x, 10                       
    0x2000|PIN_PS2_CLK, //  1: wait   0 gpio, 0                  
    0x4001, //  2: in     pins, 1                    
    0x2080|PIN_PS2_CLK, //  3: wait   1 gpio, 0                  
    0x0041, //  4: jmp    x--, 1                    
    // 0x4075, //  5: in     null, 21                   
    // 0x8020, //  6: push   block                      
};

static const struct pio_program pio_program_PS2 = {
    .instructions = pio_program_PS2_instructions,
    .length = 5,
    .origin = -1,
};

static inline pio_sm_config ps2kbd_program_get_default_config(uint offset) {
	pio_sm_config c = pio_get_default_sm_config();
	sm_config_set_wrap(&c, offset + ps2kbd_wrap_target, offset + ps2kbd_wrap);
	return c;
}

bool parity8(uint8_t data){
    bool out=1;
    for(int i=8;i--;){
      out^=(data&1);
      data>>=1;
    }
    return out;
}

static void  inInit(uint gpio){
    gpio_init(gpio);
    gpio_set_dir(gpio,GPIO_IN);
    gpio_pull_up(gpio);
}
/*
void send_bit( void ){
	uint8_t val;
	
	_bitcount++;               // Now point to next bit
	switch( _bitcount ){
		case 1: 
		#if defined( PS2_CLEAR_PENDING_IRQ ) 
			// Start bit due to Arduino bug
			digitalWrite( PIN_PS2_DATA, LOW );
			break;
		#endif
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		// Data bits
		val = _shiftdata & 0x01;   // get LSB
		digitalWrite( PIN_PS2_DATA, val ); // send start bit
		_parity += val;            // another one received ?
		_shiftdata >>= 1;          // right _SHIFT one place for next bit
		break;
		case 10:
		// Parity - Send LSB if 1 = odd number of 1's so parity should be 0
		digitalWrite( PIN_PS2_DATA, ( ~_parity & 1 ) );
		break;
		case 11: // Stop bit write change to input pull up for high stop bit
		pininput( PIN_PS2_DATA );
		break;
		case 12: // Acknowledge bit low we cannot do anything if high instead of low
		if( !( _now_send == PS2_KC_ECHO || _now_send == PS2_KC_RESEND ) )
		{
            _last_sent = _now_send;   // save in case of resend request
            _ps2mode |= _LAST_VALID;
		}
		// clear modes to receive again
		_ps2mode &= ~_TX_MODE;
		if( _tx_ready & _HANDSHAKE )      // If _HANDSHAKE done
		_tx_ready &= ~_HANDSHAKE;
		else                              // else we finished a command
		_tx_ready &= ~_COMMAND;
		if( !( _ps2mode & _WAIT_RESPONSE ) )   //  if not wait response
		send_next( );                    // check anything else to queue up
		_bitcount = 0;	            // end of byte
		break;
		default: // in case of weird error and end of byte reception re-sync
		_bitcount = 0;
	}
}

void send_now( uint8_t command ){
	_shiftdata = command;
	_now_send = command;     // copy for later to save in last sent
	#if defined( PS2_CLEAR_PENDING_IRQ ) 
		_bitcount = 0;          // AVR/SAM ignore extra interrupt
		#else
		_bitcount = 1;          // Normal processors
	#endif
	_parity = 0;
	_ps2mode |= _TX_MODE + _PS2_BUSY;
	
	// Only do this if sending a command not from Handshaking
	if( !( _tx_ready & _HANDSHAKE ) && ( _tx_ready & _COMMAND ) )
	{
		_bytes_expected = _response_count;  // How many bytes command will generate
		_ps2mode |= _WAIT_RESPONSE;
	}
	
	// STOP interrupt handler 
	// Setting pin output low will cause interrupt before ready
	detachInterrupt( digitalPinToInterrupt( PS2_IrqPin ) );
	// set pins to outputs and high
	digitalWrite( PS2_DataPin, HIGH );
	pinMode( PS2_DataPin, OUTPUT );
	digitalWrite( PS2_IrqPin, HIGH );
	pinMode( PS2_IrqPin, OUTPUT );
	// Essential for PS2 spec compliance
	delayMicroseconds( 10 );
	// set Clock LOW
	digitalWrite( PS2_IrqPin, LOW );
	// Essential for PS2 spec compliance
	// set clock low for 60us
	delayMicroseconds( 60 );
	// Set data low - Start bit
	digitalWrite( PS2_DataPin, LOW );
	// set clock to input_pullup data stays output while writing to keyboard
	pininput( PS2_IrqPin );
	// Restart interrupt handler
	attachInterrupt( digitalPinToInterrupt( PS2_IrqPin ), ps2interrupt, FALLING );
	//  wait clock interrupt to send data
}
*/

void  start_PS2_capture(){

	sm_ps2 = pio_claim_unused_sm(pio_ps2, true);
    uint offset = pio_add_program(pio_ps2, &pio_program_PS2);
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset, offset + (pio_program_PS2.length-1)); 
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    inInit(PIN_PS2_CLK);
    inInit(PIN_PS2_DATA);

    sm_config_set_in_shift(&c, true, true, 11);//??????  
    sm_config_set_in_pins(&c, PIN_PS2_DATA);

    pio_sm_init(pio_ps2, sm_ps2, offset, &c);
    pio_sm_set_enabled(pio_ps2, sm_ps2, true);
     
     uint32_t freq_sm=200000;
    float fdiv=(clock_get_hz(clk_sys)/freq_sm);//частота SM
    if (fdiv<1) fdiv=1;
    uint32_t fdiv32=(uint32_t) (fdiv * (1 << 16));
    fdiv32=fdiv32&0xfffff000;//округление делителя
    pio_ps2->sm[sm_ps2].clkdiv=fdiv32; //делитель для конкретной sm

    //инициализация DMA
    int dma_chan0 = dma_claim_unused_channel(true);
    int dma_chan1 = dma_claim_unused_channel(true);

    dma_channel_config c0 = dma_channel_get_default_config(dma_chan0);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);
    channel_config_set_read_increment(&c0, false);
    channel_config_set_write_increment(&c0, true);

    uint dreq=DREQ_PIO1_RX0+sm_ps2;
    if (pio_ps2==pio0) dreq=DREQ_PIO0_RX0+sm_ps2;
    channel_config_set_dreq(&c0, dreq);
   
    channel_config_set_chain_to(&c0, dma_chan1);                      

    uint32_t* addr_read_dma= (uint32_t*)&pio_ps2->rxf[sm_ps2];

    dma_channel_configure(
        dma_chan0,
        &c0,
        &PS_2_BUS[0], // Write address 
        addr_read_dma,             //  read address
        LEN_PS2_BUF, // 
        false            // Don't start yet
    );

    dma_channel_config c1 = dma_channel_get_default_config(dma_chan1);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
    channel_config_set_read_increment(&c1, false);
    channel_config_set_write_increment(&c1, false);
    // channel_config_set_chain_to(&c1, dma_chan0);                      

    static uint32_t addr_write_DMA0[1];
    addr_write_DMA0[0]=(uint32_t)&PS_2_BUS[0]; 
    dma_channel_configure(
        dma_chan1,
        &c1,
        &dma_hw->ch[dma_chan0].al2_write_addr_trig, // Write address 
        &addr_write_DMA0[0],             //  read address
        1, // 
        false            // Don't start yet
    );

    for(int i=0;i<LEN_PS2_BUF;i++) PS_2_BUS[i]=0;

    dma_start_channel_mask((1u << dma_chan0)) ;
};

typedef struct kb_translate{
	 uint8_t code;
	 uint8_t bank;
	 uint32_t mask;
} __attribute__((packed)) kb_translate;

#define TRANS_TABLE_SIZE 104


kb_translate __in_flash() trans_table[TRANS_TABLE_SIZE]={ //0x0x - Not E0, 0x1x - E0, 0x2x - E1 
	//Bank 1 // service keys
	{0x4E,0x01,KB_U1_MINUS},
	{0x55,0x01,KB_U1_EQUALS},
	{0x5D,0x01,KB_U1_BACKSLASH},
	{0x66,0x01,KB_U1_BACK_SPACE},
	{0x5A,0x01,KB_U1_ENTER}, 
	{0x4A,0x01,KB_U1_SLASH}, 
	{0x0E,0x01,KB_U1_TILDE}, 
	{0x0D,0x01,KB_U1_TAB}, 
	{0x58,0x01,KB_U1_CAPS_LOCK}, 
	{0x76,0x01,KB_U1_ESC}, 
	{0x12,0x01,KB_U1_L_SHIFT},
	{0x14,0x01,KB_U1_L_CTRL},
	{0x11,0x01,KB_U1_L_ALT},
	{0x59,0x01,KB_U1_R_SHIFT},
	{0x29,0x01,KB_U1_SPACE}, 
	//Bank 0
	{0x1C,0x00,KB_U0_A},
	{0x32,0x00,KB_U0_B},
	{0x21,0x00,KB_U0_C},
	{0x23,0x00,KB_U0_D},
	{0x24,0x00,KB_U0_E},
	{0x2B,0x00,KB_U0_F},
	{0x34,0x00,KB_U0_G},
	{0x33,0x00,KB_U0_H},
	{0x43,0x00,KB_U0_I},
	{0x3B,0x00,KB_U0_J},
	{0x42,0x00,KB_U0_K},
	{0x4B,0x00,KB_U0_L},
	{0x3A,0x00,KB_U0_M},
	{0x31,0x00,KB_U0_N},
	{0x44,0x00,KB_U0_O},
	{0x4D,0x00,KB_U0_P},
	{0x15,0x00,KB_U0_Q},
	{0x2D,0x00,KB_U0_R},
	{0x1B,0x00,KB_U0_S},
	{0x2C,0x00,KB_U0_T},
	{0x3C,0x00,KB_U0_U},
	{0x2A,0x00,KB_U0_V},
	{0x1D,0x00,KB_U0_W},
	{0x22,0x00,KB_U0_X},
	{0x35,0x00,KB_U0_Y},
	{0x1A,0x00,KB_U0_Z},
	{0x54,0x00,KB_U0_LEFT_BR},
	{0x5B,0x00,KB_U0_RIGHT_BR},
	{0x4C,0x00,KB_U0_SEMICOLON},
	{0x52,0x00,KB_U0_QUOTE},
	{0x41,0x00,KB_U0_COMMA},
	{0x49,0x00,KB_U0_PERIOD},
	//Bank 1
	{0x45,0x01,KB_U1_0},
	{0x16,0x01,KB_U1_1},
	{0x1E,0x01,KB_U1_2},
	{0x26,0x01,KB_U1_3},
	{0x25,0x01,KB_U1_4},
	{0x2E,0x01,KB_U1_5},
	{0x36,0x01,KB_U1_6},
	{0x3D,0x01,KB_U1_7},
	{0x3E,0x01,KB_U1_8},
	{0x46,0x01,KB_U1_9},

	//Bank 2
	{0x70,0x02,KB_U2_NUM_0},
	{0x69,0x02,KB_U2_NUM_1},
	{0x72,0x02,KB_U2_NUM_2},
	{0x7A,0x02,KB_U2_NUM_3},
	{0x6B,0x02,KB_U2_NUM_4},
	{0x73,0x02,KB_U2_NUM_5},
	{0x74,0x02,KB_U2_NUM_6},
	{0x6C,0x02,KB_U2_NUM_7},
	{0x75,0x02,KB_U2_NUM_8},
	{0x7D,0x02,KB_U2_NUM_9},
	{0x77,0x02,KB_U2_NUM_LOCK},
	{0x7C,0x02,KB_U2_NUM_MULT},
	{0x7B,0x02,KB_U2_NUM_MINUS},
	{0x79,0x02,KB_U2_NUM_PLUS},
	{0x71,0x02,KB_U2_NUM_PERIOD},
	{0x7E,0x02,KB_U2_SCROLL_LOCK},
	//Bank 3
	{0x05,0x03,KB_U3_F1}, 
	{0x06,0x03,KB_U3_F2}, 
	{0x04,0x03,KB_U3_F3}, 
	{0x0C,0x03,KB_U3_F4}, 
	{0x03,0x03,KB_U3_F5}, 
	{0x0B,0x03,KB_U3_F6}, 
	{0x83,0x03,KB_U3_F7}, 
	{0x0A,0x03,KB_U3_F8}, 
	{0x01,0x03,KB_U3_F9}, 
	{0x09,0x03,KB_U3_F10},
	{0x78,0x03,KB_U3_F11},
	{0x07,0x03,KB_U3_F12},
	//E0 Bank 1
	{0x1F,0x11,KB_U1_L_WIN},
	{0x14,0x11,KB_U1_R_CTRL},
	{0x11,0x11,KB_U1_R_ALT},
	{0x27,0x11,KB_U1_R_WIN},
	{0x2F,0x11,KB_U1_MENU},
	//E0 Bank 2
	{0x7C,0x12,KB_U2_PRT_SCR}, //для принт скрин обработаем только 1 код
	//{0x12,0x12,KB_U2_PRT_SCR}, //для принт скрин обработаем только 1 код
	{0x4A,0x12,KB_U2_NUM_SLASH},
	{0x5A,0x12,KB_U2_NUM_ENTER},
	{0x75,0x12,KB_U2_UP},
	{0x72,0x12,KB_U2_DOWN},
	{0x74,0x12,KB_U2_RIGHT},
	{0x6B,0x12,KB_U2_LEFT},
	{0x71,0x12,KB_U2_DELETE},
	{0x69,0x12,KB_U2_END},
	{0x7A,0x12,KB_U2_PAGE_DOWN},
	{0x7D,0x12,KB_U2_PAGE_UP},
	{0x6C,0x12,KB_U2_HOME},
	{0x70,0x12,KB_U2_INSERT},
	//E1 Bank 2
	{0x14,0x22,KB_U2_PAUSE_BREAK},
	//{0x00,0x00,0x00},
};


#define RECOVER_TIME_MS 100
uint32_t recover_timer =0;
kb_u_state kb_st_ps2;

void (translate_scancode)(uint8_t code,bool is_press, bool is_e0,bool is_e1){
	if(code==0) return;
	uint8_t bank_switch=0x00;
	if(is_e0){bank_switch=0x10;};
	if(is_e1){bank_switch=0x20;};
	
	for(uint8_t idx=0;idx<TRANS_TABLE_SIZE;idx++){
		if((trans_table[idx].bank&0xF0)==bank_switch){
			//printf("idx[%03d] bank[%02X] bank_switch[%02X]\n",idx+1,trans_table[idx].bank,bank_switch);
			if((trans_table[idx].code==code)){
				////printf("idx[%03d] bank[%02X] bank_switch[%02X] code[%02X]\n",idx+1,trans_table[idx].bank,bank_switch,code);
				////printf("code>%02X	  is_press>%d    is_e0>%d    is_e1>%d    bank_switch>%02X \n",code,is_press,is_e0,is_e1,bank_switch);
				////printf("res_code>%02X    res_bank>%02X    res_mask>%08lX\n",trans_table[idx].code,(trans_table[idx].bank&0x0F),trans_table[idx].bank);
				if(is_press){
					//printf("press> idx[%03d] bank[%02X] bank_switch[%02X] code[%02X]\n",idx+1,trans_table[idx].bank,bank_switch,code);
					kb_st_ps2.u[(trans_table[idx].bank&0x0F)]|=trans_table[idx].mask;
					kb_st_ps2.state&=~0x08;
					kb_st_ps2.state|=0x80;
				}  else {
					//printf("relea> idx[%03d] bank[%02X] bank_switch[%02X] code[%02X]\n",idx+1,trans_table[idx].bank,bank_switch,code);
					kb_st_ps2.u[(trans_table[idx].bank&0x0F)]&=~trans_table[idx].mask;
					kb_st_ps2.state|=0x08;
					kb_st_ps2.state&=~0x80;
				}
				break;
			}
		}
	}
}

void (clearKeys)(){
	//printf("clearKeys\n");
	kb_st_ps2.u[0]=0;
	kb_st_ps2.u[1]=0;
	kb_st_ps2.u[2]=0;
	kb_st_ps2.u[3]=0;
	kb_st_ps2.state=0x00;	
}

uint8_t (get_scan_code)(void){
    if (PS_2_BUS[inx_rd_buf]==0) {
       // printf("SCAN_CODES=0");//test
        return 0;
     }

    uint32_t val=PS_2_BUS[inx_rd_buf];
	//printf("PS/2 rc %4.4lX (%ld)\n", val, val);
    val>>=21;
    //printf("PS/2 keycode %2.2lX (%ld)\n", (val>>1)&0xff, (val>>1)&0xff);
    if (((val&0x401)!=0x400)||((val>>9)&1)!=(parity8((val>>1)&0xff))){  //если ошибка данных ps/2
		PS_2_BUS[inx_rd_buf]=0;
		inx_rd_buf=(inx_rd_buf+1)%LEN_PS2_BUF;
		// перезапускаем SM и очищаем состояние клавиатуры
		//printf("Error PS/2. Restart SM\n");
		pio_sm_restart(pio_ps2, sm_ps2);
		memset(&kb_st_ps2,0,sizeof(kb_u_state));           
        return 0xFF;
	}
    PS_2_BUS[inx_rd_buf]=0;
    inx_rd_buf=(inx_rd_buf+1)%LEN_PS2_BUF;
    return (val>>1)&0xff;
}

bool (decode_PS2)(){
	static bool is_e0=false;
	static bool is_e1=false;
	static bool is_f0=false;
	//static char test_str[128];
	uint8_t code;
	int parced = 0;

	while((code=get_scan_code())>0){
		parced++;
		switch (code) {
			/*
			00 	Keyboard error - see ff
			aa 	BAT (Basic Assurance Test) OK
			ee 	Result of echo command
			f1 	Some keyboards, as reply to command a4:Password not installed
			fa 	Acknowledge from kbd
			fc 	BAT error or Mouse transmit error
			fd 	Internal failure
			fe 	Keyboard fails to ack, please resend
			ff 	Keyboard error 
			*/
			case 0x00:{
				return false;
			}
			case 0xFC:
			case 0xFD:			
			case 0xFE:
			case 0xFF:{
				printf("PS/2 keyboard Error\n");
				clearKeys();
				is_e0=false;
				is_e1=false;
				is_f0=false;
				code=0;
				kb_st_ps2.state=0x00;
				return false;
				break;			 
			}
			case 0xAA: {
				printf("PS/2 keyboard Self test passed\n");
				break;			 
			}
			case 0xE1: {
				is_e1=true;
				continue;
				break;
			}
			case 0xE0: {
				is_e0=true;
				continue;
				break;
			}
			case 0xF0: {
				is_f0=true;
				continue;
				break;
			}
			default: {
				translate_scancode(code,!is_f0,is_e0,is_e1);
				is_e0=false;
				if (is_f0) is_e1=false;
				is_f0=false;
				//keys_to_str(test_str,' ',kb_st_ps2,false);
				////printf("is_e0=%d, is_f0=%d, code=0x%02x test_str=%s\n",is_e0,is_f0,scancode,test_str);
				//преобразование из универсальных сканкодов в матрицу бытрого преобразования кодов для zx клавиатуры
				//zx_kb_decode(zx_keyboard_state);
				//произошли изменения
				break;
			}
		}
	}
	if(parced>0){
		return true;
	}
	//kb_st_ps2.state=0x00;
	return false;
}

static void set_pin(int PIN, bool data){
    if (data){
        gpio_init(PIN);
        gpio_set_dir(PIN,GPIO_IN);
        gpio_pull_up(PIN);
        return;
    }
    gpio_init(PIN);
    gpio_set_dir(PIN,GPIO_OUT);
    gpio_put(PIN,0);
}

void send_PS2_data(uint8_t data){

    bool  p8=parity8(data);

    set_pin(PIN_PS2_CLK,0);
    busy_wait_us(100);
    set_pin(PIN_PS2_DATA,0);
    busy_wait_us(1);

    set_pin(PIN_PS2_CLK,1);
    while(!gpio_get(PIN_PS2_CLK));

    for(int i=8;i--;){
        while(gpio_get(PIN_PS2_CLK));
        set_pin(PIN_PS2_DATA,data&1);      
        while(!gpio_get(PIN_PS2_CLK)); 
        data>>=1;
	}
    
    while(gpio_get(PIN_PS2_CLK));
    set_pin(PIN_PS2_DATA,p8);      
    while(!gpio_get(PIN_PS2_CLK)); 

    while(gpio_get(PIN_PS2_CLK));
    set_pin(PIN_PS2_DATA,1);      
    while(!gpio_get(PIN_PS2_CLK)); 


    while(gpio_get(PIN_PS2_CLK));
    while(!gpio_get(PIN_PS2_CLK)); 
    
};


/*
static uint8_t dataKB[512];


//состояние шины данных спектрума для любого адресного состояния

uint8_t* zx_keyboard_state=dataKB;//&dataKB[0x100-];

uint64_t keyboard_state=0;
//состояние клавиатурной матрицы спектрума
//static

void zx_kb_decode(uint8_t* zx_kb_state){
	//memset(zx_keys_matrix,0,8);
	static uint64_t tmp_zx_kb_state64[32];
	uint8_t* tmp_zx_kb_state8=(uint8_t*)tmp_zx_kb_state64;
	
	uint8_t zx_keys_matrix[8];
	convert_kb_u_to_kb_zx(&kb_st_ps2,zx_keys_matrix);
	
	for(int i=0;i<256;i++){
		uint8_t out8=0;
		uint8_t inx=i;
		for(int k=0;k<8;k++){
			if (!(inx&1)) out8|=zx_keys_matrix[k];
			inx>>=1;
		}
		//if (out8!=0) G_PRINTF("i=%d\n",i);//test
		//if (zx_kb_state[i]!=(~out8)) zx_kb_state[i]=(~out8);
		tmp_zx_kb_state8[i]=(~out8);
	}
	//для быстрого копирования всего буфера
	
	
	uint64_t* dst_zx_kb_state64=(uint64_t*)zx_kb_state;
	uint64_t* src_zx_kb_state64=tmp_zx_kb_state64;
	for(int i=32;i--;){
		*dst_zx_kb_state64++=*src_zx_kb_state64++;
	}
	
	
};

*/