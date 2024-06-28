                                            // ================================================== //
                                            // ===============  BCI_LMSE_ESP.cpp ================ //
                                            // ================================================== //

# include "BCI_LMSE_ESP.hpp"
//# include "pins_arduino.h"
# include <SPI.h>
# include <stdlib.h>
# include "esp32-hal-spi.h"
# include <math.h>


	float _tCLK = 0.000666; // in milliseconds = 666ns(MAX) or 414ns(MIN) for MASTER CLOCK.
	float _tSCK = (1.0/spiClk); // period of serial clock.
	float _tBG;
	float _tRST;
	float _tPOR;
	float _tSTART;
	float _tEXEC;
	float _tREAD;
	float _tUPDATE;
	float _tSDECODE;
	float _tSDECODE_TH;

SPIClass * vspi = NULL; // create an instance vspi in SPIClass.
//**** breakout ESP32 ****//
/* DRDY = 16, RESET = 13, CS = 15, MOSI = 23, MISO = 19, SCK = 18 */

void BCI_LMSE_ESP::initialize(int _ESP_DRDY, int _ESP_CS, int _ESP_RST) {

	//*************************** DECLARATIONS ************************/

	// public class variables assignment
	DRDY = _ESP_DRDY; 
	CS = _ESP_CS;
	RST = _ESP_RST;


	_tBG = 3000;	
	_tPOR = pow(2,18)*_tCLK; //Wait after power up until RESET, the min value is (2^18*tCLK);
	_tRST =  2.0*_tCLK; // Reset Low Duration , the min value is (2*tCLK);
	_tSTART = 18.0*_tCLK - _tRST; // Duration to wait before using the Device.
	_tEXEC = 8.0 * _tSCK; // time elapsed in data execution of 1 Byte = 1 command.
	_tSDECODE_TH = 4.0*_tCLK; // Necessary time (theorical) to decode any command (in byte)
	_tSDECODE = 0;

	if(_tSDECODE_TH <= _tEXEC){
		_tSDECODE = 0;
	} else { 
		_tSDECODE = _tSDECODE_TH - _tEXEC; 
	}

	pinMode(DRDY, INPUT); // to receive Data Ready signal from ADS1299 Chip.
	digitalWrite(DRDY, HIGH); // Do nothing until receiving Ready data Signal
	pinMode(CS, OUTPUT); // Output to select the Device (ADS1299).
	digitalWrite(CS, HIGH); // Device not selected in this level.

	// *************************** POWER-UP SEQUENCING ************************//
	pinMode(RST, OUTPUT); // Configure RST pin to be an OUTPUT.
	digitalWrite(RST, HIGH); // don't send a reset pulse while time < _tPOR.
	delay(_tBG); // in experimental test, 3Sec founded , the time to achieve 1.1V in VCAP1.
	delay(_tPOR + 2 *_tCLK); // necessary time to get the power-supplies stabilized, and reach their final values. 
	digitalWrite(RST, LOW); // Transmit negative pulse RESET for time=_tRST to initialize the digital portion of the ADS1299.
	delay(_tRST); // RESET Low Duration before turning to HIGH.
	digitalWrite(RST, HIGH);
	delay(_tSTART); // Recommended dalay to have all registers in their default configuration. 

// *************************** SPI settings. ***************************//
// MISO is automatically set to INPUT.
	pinMode(SCK, OUTPUT);
	pinMode(MOSI, OUTPUT);
	pinMode(MISO, INPUT);
	// pinMode(SS, OUTPUT); // pinMode(CS, OUTPUT)--> This pin is mapped with CS=15.
	
// set SPI Outputs values.
	digitalWrite(SCK, LOW);
	digitalWrite(MOSI, LOW);
	//digitalWrite(SS, HIGH); // because SS slave is active Low --> This pin is mapped with CS=15.
	

// SPI initialization

	vspi = new SPIClass(VSPI);
	vspi->begin();
	vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));

	/*-- RESET SRB2,BIAS_IN inclusion arrays and set channel settings to default */

	for (uint8_t i = 0; i<8; i++){
		SRB2_include[i] = false; 		// SRB2 inclusion.
		BIAS_SENS_routing[i] = false; 	// BIAS generation.
		defaultChannelSettings(i+1);
	}

	resetADS1299();          // Use the function to load the default parameters for ADS.
	
}

// ************************* using SRB1, SRB2 and BIAS_SENS ********************* //

void BCI_LMSE_ESP::SRB2SETTER(uint8_t numCH){
	SRB2_include[numCH-1] = true;
	channelSettings[numCH-1][CH_SRB2] = 1;
	CHnSET0 = RREG(CH1SET + numCH-1);
	bitSet(CHnSET0, 3);
	WREG(CH1SET + numCH-1, CHnSET0);
	CHnSET0 = 0x00;
}

void BCI_LMSE_ESP::SRB1SETTER(){
	SRB1_include = true;
	for(uint8_t numCH=1; numCH<=8; numCH++){
		channelSettings[numCH-1][CH_SRB1] = 1;
	}
	MISC1_buff = RREG(MISC1);
	bitSet(MISC1_buff, 5);
	WREG(MISC1, MISC1_buff);		
}

void BCI_LMSE_ESP::BIAS_SENS_ROUTING(uint8_t numCH){
	BIAS_SENS_routing[numCH-1] = true;
	channelSettings[numCH-1][CH_BIAS_ROUTING_P] = 1;
	channelSettings[numCH-1][CH_BIAS_ROUTING_N] = 1;
	CONFIGn = RREG(CONFIG3);
	CONFIGn |=BIAS_SENS_MASK; // enable internal refernce Buffer, internal BIASREF signal, BIAS buffer and BIAS sens.
	WREG(CONFIG3, CONFIGn);
	BIAS_P = RREG(BIAS_SENSP); 		// Read BIAS Positive Signal derivation Register 
	BIAS_N = RREG(BIAS_SENSN);   	// Read BIAS Negative Signal derivation Register.
	bitSet(BIAS_P, numCH-1);   		// enable the routing of numCH-th into positive BIAS derivation.
	WREG(BIAS_SENSP, BIAS_P);    	// Write the new value to BIAS_SENSP register.
	bitSet(BIAS_N, numCH-1);   		// enable the routing of numCH-th into negative BIAS derivation.
	WREG(BIAS_SENSN, BIAS_N);    	//Write the new value to BIAS_SENSN register.
	BIAS_P = BIAS_N = 0x00;
}
 
// *****************************  SYSTEM COMMANDS ********************************* //
// These Commands are: WAKEUP, STANDBY, START, STOP, RESET.
//*************** WAKEUP FUNCTION ********************** //
void BCI_LMSE_ESP::WAKEUP() {
	digitalWrite(CS, LOW); // enable the communication.
	SPI_transfer(_WAKEUP);
	digitalWrite(CS, HIGH); // end of communication.
	delay(_tSDECODE); // must wait at least (4*_tCLK) cycles before sending any other command (Datasheet, page40)
	// The WAKEUP command exits low-power standby mode
} 

//*************** STANDBY FUNCTION ********************** //
void BCI_LMSE_ESP::STANDBY() {
	digitalWrite(CS, LOW);
	SPI_transfer(_STANDBY);
	digitalWrite(CS, HIGH);
	// this is the only command to send after this mode is the:  WAKEUP.
}

//*************** RESET FUNCTION ********************** //
void BCI_LMSE_ESP::RESET(){
	digitalWrite(CS, LOW); 	// enable SPI communication.
	SPI_transfer(_RESET);  	// with: _RESET = 0b00000110  = 0x06, review the datasheet ver2017 page,40.
	digitalWrite(CS, HIGH); // stop SPI communication.
	delay(18.0*_tCLK); 	  	//at least (18*_tCLK) are required to execute the RESET command in order to complete 
						 	// initializaton of the configuration registers to default states(Datasheet pg. 35 and 41).
							//  Avoid sending any commands during this time.
}

//*************** START FUNCTION ********************** //
// There are no SCLK rate restrictions for this command and can be issued at any time.
void BCI_LMSE_ESP::START() { // This command is used to begin the conversion. 
	digitalWrite(CS, LOW); 	 // enable SPI communication
	SPI_transfer(_START);
	delay(2.0*_tCLK); 		 // it's quite to do this (datasheet page.34).
	digitalWrite(CS, HIGH);  // end of SPI communication.
}

//*************** STOP FUNCTION ************************ //
// There are no SCLK rate restrictions for this command and can be issued at any time.
void BCI_LMSE_ESP::STOP() { // This command is used to stop the conversion.
	digitalWrite(CS, LOW); // enable the communication
	SPI_transfer(_STOP);
	digitalWrite(CS, HIGH); // end of communication.
}

// *****************************  DATA READ COMMANDS ********************************* //
// These Commands are: RDATAC, SDATAC, RDATA.

// ********************* RDATAC FUNCTION ********************************//
// There are no SCLK rate restrictions for this command and can be issued at any time.
void BCI_LMSE_ESP::RDATAC() { // Read DATA continuous.
	digitalWrite(CS, LOW); // enable the communication
	SPI_transfer(_RDATAC);
	digitalWrite(CS, HIGH); // end of communication.
	delay(_tSDECODE); // see datasheet pg.41 for this restriction.
}

// ********************* SDATAC FUNCTION ********************************//
// There are no SCLK rate restrictions for this command and can be issued at any time.
void BCI_LMSE_ESP::SDATAC() { // Stop read DATA continuous
	digitalWrite(CS, LOW); // enable the communication
	SPI_transfer(_SDATAC);
	digitalWrite(CS, HIGH); // end of communication.
	delay(_tSDECODE); // wait at least 4*tCLK after this command -> see datasheet pg.42 for this restriction.
}
	
// ********************** RDATA FUNCTION ******************************************// 
// There are no SCLK rate restrictions for this command and can be issued at any time.
void BCI_LMSE_ESP::RDATA2(){ // This fonction is used to send _RDATA command.
	digitalWrite(CS, LOW);
	SPI_transfer(_RDATA);
	digitalWrite(CS, HIGH);
	delay(_tSDECODE);
} 

//********************* Reading Data on demand  RDATA MODE  *********************************/
void BCI_LMSE_ESP::RDATA() {
	resetStatusRegister(); // reset the Status register content to default values =0.
	resetChannelsData();   // reset the channles DATA buffer to default values = 0.
	
	digitalWrite(CS, LOW);  // enable the communication
	SPI_transfer(_RDATA);
	
	/*  1- Read Status Register => 03 Bytes(24bits) = [1100 + LOFF_STATP + LOFF_STATN + GPIO_Bits[7:4]] */
	for(uint8_t i = 0; i<3; i++) {
		status_reg[i] =  SPI_transfer(_pop_read);
		//delay(0.001);
	}
	/* 2- Read All 24Bit channels  */
		Serial.println(" CHANNELS DATA IN 24Bit 2'S COMPLEMENT ");
	for(uint8_t i=0; i<8; i++){
		for(uint8_t j=0; j<3; j++){
			bufferByte = SPI_transfer(_pop_read);
			channel[i] = (channel[i] << 8) | bufferByte;
		}

	}
	bufferByte = 0x00; // reset the buffer.

	digitalWrite(CS, HIGH);

	if(details){  // this will show the status register content in the debugging step.
		Serial.print("STATUS REGISTER: ");
		uint8_t li=0;
		for(uint8_t j=0; j<3; j++){
			for(uint8_t k=0; k<8; k++){
				Serial.print(bitRead(status_reg[j], 7-k), BIN);
				++li;
				if(li!=24) {
					Serial.print(", ");
				}
			}
		}

	Serial.println("");

	for(uint8_t i = 0; i<8; i++){
		uint8_t li =0 ;
		Serial.print("CH 0");
		Serial.print(i+1);
		Serial.print(" : ");
		for(uint8_t j=0; j<32; j++){
			li++;
			Serial.print(bitRead(channel[i], 31-j), BIN);
			if(li!=32) {
				Serial.print(", ");
			}
		}
		Serial.println("");
	}
	}

/* **************************** Data reformating & conversion into 32bit 2's complement ********************************* */ 
	Serial.println(" CHANNELS DATA IN 32Bit 2'S COMPLEMENT ");
	for (uint8_t i=0; i<8; i++){
		if(bitRead(channel[i], 23) == 0){
			channel[i] = channel[i] & 0x00FFFFFF;
		}else{
			channel[i] = channel[i] | 0xFF000000;
		}
	}
	if(details){
		
		for(uint8_t i = 0; i<8; i++){
			uint8_t li =0 ;
			Serial.print("CH 0");
			Serial.print(i+1);
			Serial.print(" : ");
			for(uint8_t j=0; j<32; j++){
				li++;
				Serial.print(bitRead(channel[i], 31-j), BIN);
				if(li!=32) {
					Serial.print(", ");
				}
			}
			Serial.println("");
		}
	}

}

// ************************************************************** //
void BCI_LMSE_ESP::updateChannels(boolean _32bit_2comp_format) {
	//resetStatusRegister();
	//resetChannelsData();
	digitalWrite(CS, LOW); // Start SPI communication.
	
				/**** READ CHANNEL DATA FROM ADS1299 ****/
// **** 1- pop the 24bit Status Register
	for (uint8_t i=0; i<3; i++){
		status_reg[i] = SPI_transfer(_pop_read);
	}
	
	for(uint8_t i=0; i<8; i++){
		for(uint8_t j=0; j<3; j++) {
			bufferByte = SPI_transfer(_pop_read);
			//channelDataRaw[counter] = bufferByte; // raw data come here.
			channel[i] = (channel[i]<<8) | bufferByte; // int data come here.
			//counter++;
		}
	}
	digitalWrite(CS, HIGH); // end of communication.

	// ***************************** Data reformating ********************************* //
	if(_32bit_2comp_format == true){
		for (uint8_t i=0; i<8; i++){
			if(bitRead(channel[i], 23) == 0){
				channel[i] = channel[i] & 0x00FFFFFF;
			} else {
				channel[i] = channel[i] | 0xFF000000;
			}
		}
	}
	bufferByte = 0x00;
}

void BCI_LMSE_ESP::allChannelsTxTPrint(long sample){
	if(sample <= 0){
		if(details){
			Serial.println("Nothing to display!");
		}
		;
	}else{
		Serial.print(sample);
		Serial.print(",");
	}
	uint8_t CH = 0;
	do {
		Serial.print(channel[CH]);
		if(CH!=7){
			Serial.print(",");
		}
		CH++;

	}while(CH<8);
	Serial.println();
}
void BCI_LMSE_ESP::oneChannelTxTPrint(uint8_t numCH, long sample){
	if(sample <= 0)
		{
			if(details)
				{
					Serial.println("Nothing to display!");
				}
					;
	}else{
		Serial.print(sample);
		Serial.print(", ");
	}
	
	Serial.print(channel[numCH-1]);
	Serial.println("");
}

// *************************** DATA SENDER THROUGH SERIAL PORT **************************//

void BCI_LMSE_ESP::BinaryDataSenderToSerial(uint8_t bci_dimension,long int sample){

	// header
	Serial.write((byte)HEADER_FRAME);
	//Serial.write((1+bci_dimension)*4); // amount (in 1-Byte) of transmitted data, expressed in Bytes.
						   // e.g if N = 8 => Amount = 9*4=36-Byte = 288-bit <=> (8CH + 1STAT * 4).
	// send the sample number.
	buff1 = sample;	// long int<=>32-bit(4-Byte)[buff_val1]=long int<=>32-bit(4-Byte)[buff_val2]. 
	Serial.write(buff2, 4); // Byte buff_val2 <=> (8*4)  ---> send 4-Byte = 32-bit.
	for(uint8_t CH = 1; CH<=bci_dimension; CH++){
		buff1 = channel[CH-1];
		Serial.write(buff2, 4); //send 4 Bytes in 1 Shot.
	}
	// Footer
	Serial.write((byte)FOOTER_FRAME);
}

// ****************** other Data Binary Format *************************** //
// ************** 1: ModularEEG --> P2 protocol (program: BrainBay).
// in this case we have to send only 6-channels of data, per the P2 protocol.

void BCI_LMSE_ESP::dataSenderToSerialAsOpenEEG_brainBay(long signal_samples){
	static int count = -1;
	sync0 = 0xA5;
	sync1 = 0x5A;
	version = 0x02;

	Serial.write(sync0);
	Serial.write(sync1);
	Serial.write(version);

	foo = (byte) signal_samples;
	if(foo == sync0) foo--;
	Serial.write(foo);

	for(uint8_t CH=1; CH <=6 ;CH++){
		buff_val32 = channel[CH-1];

		// prepare value to the transmission.
	
		buff_val32 = buff_val32 / (32);
		buff_val32 = constrain(buff_val32, min_modular_EEG_int16, max_modular_EEG_int16);
		buff_val1_u16 = (unsigned int)(buff_val32 & (0x0000FFFF));
		if(buff_val1_u16 > 1023) buff_val1_u16 = 1023;

		foo = (byte)((buff_val1_u16 >> 8) & (0x00FF));
		if(foo == sync0){
			foo--;
		}
		Serial.write(foo);
		foo = (byte)(buff_val1_u16 & 0x00FF);
		if(foo == sync0) foo--;
		Serial.write(foo);
	}
	
	switches = 0x07;
	count++;
	if(count >=18){
		 count = 0;
	}
	if(count >= 9){
		switches = 0x0F;
	}

	Serial.write(switches);
}

// ********************** DATA Acquisition Streaming Functions *********************** //
// *************************    START STREAMING  ***************************** //
void BCI_LMSE_ESP::resetADS1299(){
	resetStatusRegister();
	resetChannelsData();
	//RESET();
	SDATAC();         // this command is very important, because in RDATAC mode the RREG will be ignored.
	details = WITHOUT;
	for (uint8_t i=1; i<=8; i++){
		powerDownChannels(i, false);
	}
	daisyChainModeTest();     	  				// Set to Daisy-chain mode or multiple readback.
	dataRateSettings(DR_250);        			// Keep data rate to default settings 250SPS.
	testSignalSources(TEST_SIG_EXTERNAL);   	// if there is no external test signal, keep to generate the internal one if 1.
	testSignalAmplitude(TEST_SIG_AMP_1);   		// This determines the calibration signal amplitude = -1*(VREFP - VREFN)/2400.
	testSignalFrequency(TEST_SIG_FCLK_21);		// This determines the calibration signal frequency =fCLK/2^21.
	pdInternalReferenceBuffer(INTERNAL_REF_BUFFER);	// power ON/DOWN the internal reference buffer, default = internal reference buffer.
	//delay(100); 								// required time to settle the internal reference.
	biasSignalReference(INTERNAL_BIAS_REF); 	// external or internal BIAS source.
	biasBufferPower(BIAS_BUFFER_ON);  			// Power ON bias buffer
	//   in Future work we would to introduce the lead off detection && the impedance measurements functions	
	isADSWorking = false;
	//SDATAC(); // not necessary because it was been set above.
	details = WITH;
} 

void BCI_LMSE_ESP::daisyChainModeTest(){
	CONFIGn = RREG(CONFIG1);
	if(daisy_chain_mode==false){
		bitSet(CONFIGn, 6);
		WREG(CONFIG1, CONFIGn);
		multiple_readback_mode = true;
	}else{
		bitClear(CONFIGn, 6);
		WREG(CONFIG1, CONFIGn);
		multiple_readback_mode = false;
	}
	CONFIGn = 0x00; // Reset CONFIGn
}

void BCI_LMSE_ESP::dataRateSettings(int _DR){
	CONFIGn = RREG(CONFIG1);
	switch(_DR){
		case DR_16K:
			if(details) Serial.println("Set Data Rate to 16KSPS.");
			CONFIGn = (CONFIGn & DATA_RATE_OPS_MASK) | DATA_RATE_16K_MASK;
			break;
		case DR_8K:
			if(details) Serial.println("Set Data Rate to 8KSPS.");
			CONFIGn = (CONFIGn & DATA_RATE_OPS_MASK) | DATA_RATE_08K_MASK;
			break;
		case DR_4K:
			if(details) Serial.println("Set Data Rate to 4KSPS.");
			CONFIGn = (CONFIGn & DATA_RATE_OPS_MASK) | DATA_RATE_04K_MASK;
			break;
		case DR_2K:
			if(details) Serial.println("Set Data Rate to 2KSPS.");
			CONFIGn = (CONFIGn & DATA_RATE_OPS_MASK) | DATA_RATE_02K_MASK;
			break;
		case DR_1K:
			if(details) Serial.println("Set Data Rate to 1KSPS.");
			CONFIGn = (CONFIGn & DATA_RATE_OPS_MASK) | DATA_RATE_01K_MASK;
			break;
		case DR_500:
			if(details) Serial.println("Set Data Rate to 500SPS.");
			CONFIGn = (CONFIGn & DATA_RATE_OPS_MASK) | DATA_RATE_500_MASK;
			break;
		case DR_250:
			if(details) Serial.println("Set Data Rate to 250SPS.");
			CONFIGn = (CONFIGn & DATA_RATE_OPS_MASK) | DATA_RATE_250_MASK;
			break;
		default:
			if(details) Serial.println("No such Data Rate !");
			break;
	}
	WREG(CONFIG1, CONFIGn);
	CONFIGn = 0x00; // reset CONFIGn
}

void BCI_LMSE_ESP::pdInternalReferenceBuffer(int _pd_ref_buffer){
	CONFIGn = RREG(CONFIG3);
	switch(_pd_ref_buffer){
		case EXTERNAL_REF_BUFFER:	
			CONFIGn = (CONFIGn & PD_REFERENCE_MASK) | PD_REFERENCE_EXTERN_MASK;
			break;
		case INTERNAL_REF_BUFFER:
			CONFIGn = (CONFIGn & PD_REFERENCE_MASK) | PD_REFERENCE_INTERN_MASK;
			break;
		default:
			Serial.println("No such PD buffer configuraion !");
			break;
	}
	WREG(CONFIG3,CONFIGn);
	CONFIGn = 0x00; // reset CONFIGn buffer.
}

void BCI_LMSE_ESP::testSignalSources(int _test_source){
	CONFIGn = RREG(CONFIG2);
	switch(_test_source){
		case TEST_SIG_EXTERNAL:
			CONFIGn = (CONFIGn & TEST_SIGNAL_SOURCE_MASK) | TEST_SIGNAL_EXTERN_MASK;
			break;
		case TEST_SIG_INTERNAL:
			CONFIGn = (CONFIGn & TEST_SIGNAL_SOURCE_MASK) | TEST_SIGNAL_INTERN_MASK;
			break;
		default:
			Serial.println("No such test sources !");
			break;
	}
	WREG(CONFIG2, CONFIGn);
	CONFIGn = 0x00; // reset CONFIGn buffer.
}

void BCI_LMSE_ESP::testSignalAmplitude(int _test_ampli){
	CONFIGn = RREG(CONFIG2);
	switch(_test_ampli){
		case TEST_SIG_AMP_1:
			CONFIGn = (CONFIGn & TEST_SIGNAL_AMP_MASK) | TEST_SIGNAL_SIMPLE_MASK;
			break;
		case TEST_SIG_AMP_2:
			CONFIGn = (CONFIGn & TEST_SIGNAL_AMP_MASK) | TEST_SIGNAL_DOUBLE_MASK;
			break;
		default:
			Serial.println("No such test amplitude !");
			break;
	}
	WREG(CONFIG2, CONFIGn);
	CONFIGn = 0x00; // reset CONFIGn buffer.
}

void BCI_LMSE_ESP::testSignalFrequency(int _test_freq){
	CONFIGn = RREG(CONFIG2);
	switch(_test_freq){
		case TEST_SIG_FCLK_21:
			CONFIGn = (CONFIGn & TEST_SIGNAL_FREQ_MASK) | TEST_SIGNAL_FREQ21_MASK;
			break;
		case TEST_SIG_FCLK_20:
			CONFIGn = (CONFIGn & TEST_SIGNAL_FREQ_MASK) | TEST_SIGNAL_FREQ20_MASK;
			break;
		case TEST_SIG_FCLK_AT_DC:
			CONFIGn = (CONFIGn & TEST_SIGNAL_FREQ_MASK) | TEST_SIGNAl_DC_FREQ0_MASK;
			break;
		default:
		Serial.println("No such calibration frequency !");
			break;
	}
	WREG(CONFIG2, CONFIGn);
	CONFIGn = 0x00; // reset CONFIGn buffer.
}

void BCI_LMSE_ESP::temperatureMeasurement(uint8_t numCH, int gain_selection){

	channelSettings[numCH-1][CH_GAIN] = (int) pga_gain_settings;
	CHnSET0 = RREG(CH1SET + numCH-1);
	CHnSET0 = (CHnSET0 & INPUT_SELECT_MASK) | INPUT_TEMP_SENS_MASK;

	switch(gain_selection){
		case PGA_GAIN1_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN1_MASK << 4); break;
		case PGA_GAIN2_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN2_MASK << 4); break;
		case PGA_GAIN4_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN4_MASK << 4); break;
		case PGA_GAIN6_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN6_MASK << 4); break;
		case PGA_GAIN8_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN8_MASK << 4); break;
		case PGA_GAIN12_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN12_MASK << 4); break;
		case PGA_GAIN24_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN24_MASK << 4);break;
		default:
			Serial.println("WRONG PGA GAIN VALUE ..");
			pga_gain_settings =(uint8_t) ( PGA_GAIN1_MASK << 4); 
			break;
	}

	CHnSET0 = (CHnSET0 & GAIN_SET_MASK) | (byte) pga_gain_settings;
	WREG(CH1SET+numCH-1, CHnSET0);

	channelSettings[numCH-1][CH_INPUT_SELECT] =(int) INPUT_TEMP_SENS_MASK;
	channelSettings[numCH-1][CH_GAIN] =(int) (pga_gain_settings >> 4);
	CHnSET0 = 0x00;
}


void BCI_LMSE_ESP::performTestSignal(uint8_t numCH, int gain_selection, int _signal_source, int _signal_amp, int _signal_freq){
	//powerDownChannels(numCH);
	
	CHnSET0 = RREG(CH1SET + numCH-1);
	CHnSET0 = (CHnSET0 & INPUT_SELECT_MASK) | INPUT_TEST_SIGNAL_MASK;
	switch(gain_selection){
		case PGA_GAIN1_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN1_MASK << 4); break;
		case PGA_GAIN2_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN2_MASK << 4); break;
		case PGA_GAIN4_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN4_MASK << 4); break;
		case PGA_GAIN6_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN6_MASK << 4); break;
		case PGA_GAIN8_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN8_MASK << 4); break;
		case PGA_GAIN12_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN12_MASK << 4); break;
		case PGA_GAIN24_SELECT:
			pga_gain_settings =(uint8_t) ( PGA_GAIN24_MASK << 4);break;
		default:
			Serial.println("WRONG PGA GAIN VALUE ..");
			pga_gain_settings =(uint8_t) ( PGA_GAIN1_MASK << 4); 
			break;
	}

	CHnSET0 = (CHnSET0 & GAIN_SET_MASK) | (byte) pga_gain_settings;
	WREG(CH1SET + numCH-1, CHnSET0);
	channelSettings[numCH-1][CH_INPUT_SELECT] =(int) INPUT_TEST_SIGNAL_MASK;
	channelSettings[numCH-1][CH_GAIN] =(int) (pga_gain_settings >> 4);
	testSignalSources(_signal_source);
	testSignalAmplitude(_signal_amp);
	testSignalFrequency(_signal_freq);
	//powerOnChannels(numCH, (uint8_t) INPUT_TEST_SIGNAL_MASK, gain_selection);
	CHnSET0 = 0x00;
}

void BCI_LMSE_ESP::biasSignalReference(int _bias_ref){
	CONFIGn = RREG(CONFIG3);
	switch(_bias_ref){
		case EXTERNAL_BIAS_REF: // BIASREF fed externally.
			CONFIGn = (CONFIGn & BIAS_RFERENCE_MASK) | BIAS_REFERENCE_EXTERN_MASK;
			break;
		case INTERNAL_BIAS_REF: // // used to generate an internal BIASREF signal = (AVDD + AVSS)/2.
			CONFIGn = (CONFIGn & BIAS_RFERENCE_MASK) | BIAS_REFERENCE_INTERN_MASK;
			break;
		default:
			break;
	}
	WREG(CONFIG3, CONFIGn);
	CONFIGn = 0x00; // reset CONFIG3 buffer.
}

void BCI_LMSE_ESP::biasBufferPower(int pd_BIAS_buffer){
	CONFIGn = RREG(CONFIG3);
	switch(pd_BIAS_buffer){
		case BIAS_BUFFER_OFF:
			CONFIGn = (CONFIGn & PD_BIAS_BUFFER_MASK) | PD_BIAS_BUFFER_OFF_MASK;
			break;
		case BIAS_BUFFER_ON:
			CONFIGn = (CONFIGn & PD_BIAS_BUFFER_MASK) | PD_BIAS_BUFFER_ON_MASK;
			break;
		default:
			break;
	}
	WREG(CONFIG3, CONFIGn);
	CONFIGn = 0x00;
}

void BCI_LMSE_ESP::biasSignalMeasurements(uint8_t numCH){
	CONFIGn = RREG(CONFIG3);
	CHnSET0 = RREG(CH1SET+numCH-1);
	//BIAS_P  = RREG(BIAS_SENSP);
	//BIAS_N  = RREG(BIAS_SENSN);
	if(signal_bias_in_meas == true){
		bitSet(CONFIGn, 4);
		bitSet(CONFIGn, 2);
		if(internal_biasref_signal == true){
			bitSet(CONFIGn, 7);
			bitSet(CONFIGn, 3); // this setting is used in conjonction with other bits.
		}else{
			bitClear(CONFIGn, 3);
			bitClear(CONFIGn, 7);
		}
		CHnSET0 = (CHnSET0 & INPUT_SELECT_MASK) | INPUT_BIAS_MEAS_MASK;
		WREG(CH1SET+numCH-1, CHnSET0); // set the first 3bits of CHnSET for BIAS Measurements.
		CHnSET0 = 0x00; // reset CHnSET0 buffer.
		BIAS_P = 0xFF; // following datasheet p.28 p.29: BIAS signal is generated from all channels ...
		BIAS_N = 0xFF; // ... except the channel that is used for BIAS derivation or is used for BIAS measurements.
		bitClear(BIAS_P, numCH-1); // disable the positive side of channel (numCH-1) from BIAS generation.
		bitClear(BIAS_N, numCH-1); // also must disable the negative side of channel (numCH-1) from BIAS generation.
		WREG(BIAS_SENSP, BIAS_P); // write new settings for P-side.
		WREG(BIAS_SENSN, BIAS_N); // write new settings for N-side.
		channelSettings[numCH-1][CH_INPUT_SELECT] = (int) INPUT_BIAS_MEAS_MASK;
		for(uint8_t j=0; j < 8; j++){
			if(j != numCH-1){
				channelSettings[j][CH_BIAS_ROUTING_P] = 1;
				channelSettings[j][CH_BIAS_ROUTING_N] = 1;
			}else{
				channelSettings[j][CH_BIAS_ROUTING_P] = 0;
				channelSettings[j][CH_BIAS_ROUTING_N] = 0;
			}
		}
		
	}else{
		bitClear(CONFIGn, 4);
	}
	WREG(CONFIG3, CONFIGn);
	CHnSET0 = 0x00; // reset CHnSET0 buffer.
	BIAS_N  = BIAS_P = 0x00;  // reset BIAS_N/BIAS_P buffers
	CONFIGn = 0x00; // reset CONFIGn buffer.
}

void BCI_LMSE_ESP::biasSensFunction(){ // to be revized.
	CONFIGn = RREG(CONFIG3);
	if(bias_loff_sens == false){
		bitClear(CONFIGn, 1);
	}else{
		bitSet(CONFIGn, 1);
	}
	WREG(CONFIG3, CONFIGn);
	CONFIGn = 0x00; 
}

void BCI_LMSE_ESP::biasLeadOffState(){ // to be revized.
	CONFIGn = RREG(CONFIG3);
	if(bitRead(CONFIGn, 0) == 0){
		Serial.println("BIAS is Connected");
	}else{
		Serial.println("BIAS is not Connected !");
	}
	CONFIGn = 0x00;
}

// ******************    STOP STREAMING  *********************** //
void BCI_LMSE_ESP::stopADSStreaming(){
	isADSWorking =false;
	STOP(); // Stop continuous Conversion.
	delayMicroseconds(100);
	SDATAC(); // Stop read Data Continuous mode.
	resetStatusRegister();
	resetChannelsData();
}

void BCI_LMSE_ESP::startADSStreaming(){
	resetStatusRegister();
	resetChannelsData();
	CONFIGn = RREG(CONFIG4);
	bitClear(CONFIGn, 3); // set bit3 = 0 -> avoid to be in single shot mode.
	WREG(CONFIG4, CONFIGn);
	CONFIGn = 0x00; // reset the register.           
	RDATAC(); // Enter Read Data Continuous (Continuous Reading mode).
	delay(_tSDECODE);
	START(); //  Begin the continuous conversion (Conversion Mode).
	isADSWorking = true;
}

int BCI_LMSE_ESP::isDataReady(){
	return(!(digitalRead(DRDY)));
}

// *****************************  REGISTER READ and WRITE COMMANDS ********************************* //
// These Commands are: RREG, WREG.

// ******************************  DEVICE ID  ********************************************************//
byte BCI_LMSE_ESP::ADS1299ID() {
		byte data = RREG(ID);
	return data;
}

void BCI_LMSE_ESP::ADS1299ID2() {
	digitalWrite(CS, LOW);
	//SPI_transfer(_SDATAC);
	SPI_transfer(_BYTE1_RREG | 0x00);
	SPI_transfer(_pop_read);
	byte data = SPI_transfer(_pop_read);
	SPI_transfer(_RDATAC);
	digitalWrite(CS, HIGH);
	Serial.print("Device ID: ");
	Serial.print("0x00, ");
	Serial.print("0x");
	Serial.print(data, HEX);
	Serial.print(", ");
	for(uint8_t i=0; i<8; i++){
		Serial.print(bitRead(data, 7-i));
		if(i!=7) Serial.print(", ");
	}
	Serial.print("\n");
	Serial.println("-------------------------------------------------");
}


// ********************** RREG FUNCTION for Single REGISTER ******************************************// 
byte BCI_LMSE_ESP::RREG(byte _regAdd) {
	mirrorInitialize();
	byte _second_command = 0x00; // it's the number of rgisters to read (1) - 1 = (0)
	byte _first_command = _BYTE1_RREG | _regAdd; // has format (00100000 + 000rrrrr)
	SDATAC();	            //must be present at the end of process initialization to stop RDATAC.
							// else we must STOP RDATAC (SDATAC) in the main Sketch -->(ARDUINO-IDE) or another GUI.
	digitalWrite(CS, LOW); 	// enable communication, CS must be low during all RREG command.
	SPI_transfer(_first_command); 			// command to point to first register address to begin from.
	delay(_tSDECODE); 				// Command byte decode Time --> check if must be uncommented. (follow datasheet)
	SPI_transfer(_second_command);			// 2nd Command = 0x00, in the case of single register.
	delay(_tSDECODE);				// Command byte decode Time --> check if must be uncommented. (follow datasheet)
	mirror[_regAdd] = SPI_transfer(_pop_read); // only One register to read => _pop_read = 1-1= 0. 
	digitalWrite(CS, HIGH); 					// end of SPI communication while mirror contain data transfered from register which address is = _regAdd
	if(details) {
		registerNameDisplay(_regAdd);
		DecToHex(_regAdd);
		Serial.print(", ");
		DecToHex(mirror[_regAdd]);
		Serial.print(", ");
		for(byte j = 0; j<8; j++){
			Serial.print(bitRead(mirror[_regAdd], 7-j), BIN);
			if(j!=7) Serial.print(", ");
		}
		Serial.println("");
		Serial.println("-------------------------------------------------");
	}
	//RDATAC(); // you mus use it from main sketch.
	return mirror[_regAdd];			// return requested register value
}

// ********************** RREG FUNCTION for Multiple REGISTERS ******************************************// 
void BCI_LMSE_ESP::RREGS(byte _regAdd, byte _second_command) {
	mirrorInitialize();
    byte _first_command = _BYTE1_RREG | _regAdd;    //  RREG expects 001rrrrr where rrrrr = _address
    if(details){
    	Serial.println("DEBUG MODE, Printing all register states ...");
    }
    SDATAC();	            //must be present at the end of process initialization to stop RDATAC.
							// else we must STOP RDATAC (SDATAC) in the main Sketch -->(ARDUINO-IDE) or another GUI.
    digitalWrite(CS, LOW); 			//  open SPI communication
    SPI_transfer(_first_command); 	// Command and register address.		   	
    delay(_tSDECODE);		        // Command byte decode Time, datasheet page. 43. --> check if must be uncommented. (follow datasheet)		
    SPI_transfer(_second_command-1);	// specifies the number of registers to read -1 --> kind of an OFFSET.
        for(byte i = 0; i < _second_command ;i++) {
        	  mirror[_regAdd + i] = SPI_transfer(_pop_read); 	//  add register byte to mirror array
        	  delay(_tSDECODE); // Command byte decode Time, datasheet page. 43. --> check if must be uncommented. (follow datasheet)
        }

	digitalWrite(CS, HIGH); 		//  close SPI

	if(details){	            	//  more detailed diplay
		for( byte i = 0; i< _second_command; i++){
			registerNameDisplay(_regAdd + i);
			DecToHex(_regAdd + i);
			Serial.print(", ");
			DecToHex(mirror[_regAdd + i]);
			Serial.print(", ");
			for(int j = 0; j<8; j++){
				Serial.print(bitRead(mirror[_regAdd + i], 7-j), BIN);
				if(j!=7) Serial.print(", ");
			}
			Serial.print("\n");
			
		}
		Serial.println("*---------***-----------***----------***---------*");
    }
    // RDATAC(); --> this enters the device in RDARAC mode and it can't accept another RREG if uncommented.
}

// ********************** WREG FUNCTION for Single Register ******************************************// 
void BCI_LMSE_ESP::WREG(byte _regAdd, byte _value) {	//  One register to write at (_regAdd) address.
	mirrorInitialize();
    byte _first_command = _BYTE1_WREG | _regAdd; 		//  WREG expects 010rrrrr where rrrrr = _regAdd
    byte _second_command = 0x00; 						//  It's the number of rgisters to write (1-1)=(0).
    digitalWrite(CS, LOW); 								//  SPI communication enabled.
    SPI_transfer(_first_command);						//  Send WREG command & address
    delay(_tSDECODE); 
    SPI_transfer(_second_command);						//	Send number of registers to read -1
    delay(_tSDECODE); 						            // Command byte decode Time, datasheet page. 43. --> check if must be uncommented. (follow datasheet)
    SPI_transfer(_value);								//  Write the value to the register
    digitalWrite(CS, HIGH);								//  SPI communication closed.1
	mirror[_regAdd] = _value;							//  update the mirror array
	if(details){ 										//  more detailed display
		Serial.print(F("Register "));
		registerNameDisplay(_regAdd);
		DecToHex(_regAdd);
		Serial.print(F(" is correctly setting to: 0x"));
		if(_value<=0xF){
			Serial.print("0");
		}
		Serial.println(_value, HEX);
		Serial.println("-------------------------------------------------");
	}
}

// ********************** WREG FUNCTION for Multiple REGISTERS ******************************************// 

void BCI_LMSE_ESP::WREGS(byte _regAdd, byte _second_command){
	byte _first_command = _BYTE1_WREG | _regAdd; 			// WREG expects 0x010rrrrr, where rrrrr = _regAdd
	digitalWrite(CS, LOW);									// enable SPI Communication.
	SPI_transfer(_first_command);							// Send the first Byte (Command + Starting Register Address).
	delay(_tSDECODE); 								// Command byte decode Time, datasheet page. 43. --> check if must be uncommented. (follow datasheet)
	SPI_transfer(_second_command-1);							// Send the number of registers to write - 1.
	byte i = _regAdd;
	do {
		SPI_transfer(mirror[i]); 							// Write to registers from data saved in mirror.
		i++;
	} while(i<(_second_command));
	digitalWrite(CS, HIGH); 								// end of the SPI communication.
	if (details) {
		Serial.println("-------------------------------------------------");
		Serial.print(F("Registers "));
		registerNameDisplay(_regAdd);
		DecToHex(_regAdd);
		Serial.print(F(" To "));
		registerNameDisplay(_regAdd + _second_command);
		DecToHex(_regAdd + _second_command);
		Serial.println(F(" are correctly setting."));
		Serial.println("-------------------------------------------------");
	}
}

//******************* String-Byte Converters ***********************************//
void BCI_LMSE_ESP::registerNameDisplay(byte _regAdd) {
	switch(_regAdd){
		case ID:
			Serial.print("Device ID: ");
			break;
		case CONFIG1:
			Serial.print("CONFIG1: ");
			break;
		case CONFIG2:
			Serial.print("CONFIG2: ");
			break;
		case CONFIG3:
			Serial.print("CONFIG3: ");
			break;
		case LOFF:
			Serial.print("LOFF: ");
			break;
		case CH1SET:
			Serial.print("CH1SET: ");
			break;
		case CH2SET:
			Serial.print("CH2SET: ");
			break;
		case CH3SET:
			Serial.print("CH3SET: ");
			break;
		case CH4SET:
			Serial.print("CH4SET: ");
			break;
		case CH5SET:
			Serial.print("CH5SET: ");
			break;
		case CH6SET:
			Serial.print("CH6SET: ");
			break;
		case CH7SET:
			Serial.print("CH7SET: ");
			break;
		case CH8SET:
			Serial.print("CH8SET: ");
			break;
		case BIAS_SENSP:
			Serial.print("BIAS_SENSP: ");
			break;
		case BIAS_SENSN:
			Serial.print("BIAS_SENSN: ");
			break;
		case LOFF_SENSP:
			Serial.print("LOFF_SENSP: ");
			break;
		case LOFF_SENSN:
			Serial.print("LOFF_SENSN: ");
			break;
		case LOFF_FLIP:
			Serial.print("LOFF_FLIP: ");
			break;
		case LOFF_STATP:
			Serial.print("LOFF_STATP: ");
			break;
		case LOFF_STATN:
			Serial.print("LOFF_STATN: ");
			break;
		case GPIO_ADS:
			Serial.print("GPIO_ADS: ");
			break;
		case MISC1:
			Serial.print("MISC1: ");
			break;
		case MISC2:
			Serial.print("MISC2: ");
			break;
		case CONFIG4:
			Serial.print("CONFIG4: ");
			break;
		default:
			break;
	}
}

/// ************************************* ADS MANAGER ****************************//
// 1° ***  Deactivate/Activate Channels *** /
void BCI_LMSE_ESP::powerDownChannels(uint8_t numCH, boolean closeSRB2){ // numCH is the channel number.

	//byte start_CH = 1;     // strating channels number. 
	//byte end_CH = 8;       // end channels number.
	SDATAC();                // to stop the default RDATAC mode.
	//delay(1);
	CHnSET0 = RREG(CH1SET + (byte)(numCH-1)); 	// is to read the current channel setting.
	CHnSET0 &= INPUT_SELECT_MASK;       // isolate the first three bits. 
	CHnSET0 |= INPUT_SHORTED_MASK; 		// When powering down a channel TI recommends that the channel.
									    // be set ti input short mode by setting the appropriate MUXn[2:0].
										// to 001 of the CHnSET register --> we used here.
										// INPUT_SHORTED_MASK.
	// ********************This parts of code must be checked ****************************
	/*BIAS_SENS_routing[numCH-1] = false;
	SRB1_include = false;
	SRB2_include[numCH-1] = false;*/
	//*******************************************************************
	if (closeSRB2 == false)
	{
		CHnSET0 &= ~(SRB2_CH_MASK); // to open the SRB2 connection. (bitClear(CHnSET0, 3)-> it does the same thing).
	} else {
		CHnSET0 |= SRB2_CH_MASK; // to close the SRB2 connection. (bitSet(CHnSET0, 3)-> it does the same thing).
	}
	
	CHnSET0 |= INPUT_OFF_MASK;  // power-Down the channel numCH. (bitSet(CHnSET0, 7) -> it does the same thing).
	uint8_t i = 4;
	do {    
		bitClear(CHnSET0, i);  // to reduce automatically the PGA gain to value 1.
		i++;
	} while(i <= 6);
	i = 0;

	WREG(CH1SET + (byte)(numCH-1), CHnSET0); // write the new value to CHnSET register(s).

	channelSettings[numCH-1][CH_PWR_MODE] = (int) (INPUT_OFF_MASK >> 7); // set the channelSettings array at channel power-down level.
	channelSettings[numCH-1][CH_GAIN] = (int) PGA_GAIN1_MASK;	// set the channelSettings array at channel PGA gain level.
	channelSettings[numCH-1][CH_SRB2] = (int) bitRead(CHnSET0, 3); // set the channelSettings array at channel SRB2 level
	channelSettings[numCH-1][CH_INPUT_SELECT] = (int) INPUT_SHORTED_MASK; // set the channelSettings array at channel input selection level.
	channelSettings[numCH-1][BIAS_MEAS] = 0;
	channelSettings[numCH-1][CH_SRB1] = (int) bitRead(RREG(MISC1), 5); // set the channelSettings array at SRB1 state.

	// Next its necessary to disconnect any channels from BIAS voltage drivation NEGATIVE and POSITIVE.
	if (readOnce == true){
		BIAS_P = RREG(BIAS_SENSP); 	// Read BIAS Positive Signal derivation Register.
		BIAS_N = RREG(BIAS_SENSN);  // Read BIAS Negative Signal derivation Register.

		readOnce = false;
	}

	bitClear(BIAS_P, numCH-1);  // disable the routing of numCH-th into positive BIAS derivation.
	WREG(BIAS_SENSP, BIAS_P);  // Write the new value to BIAS_SENSP register.
	bitClear(BIAS_N, numCH-1);  // disable the routing of numCH-th into negative BIAS derivation.
	WREG(BIAS_SENSN, BIAS_N);  // Write the new value to BIAS_SENSN register.

	channelSettings[numCH-1][CH_BIAS_ROUTING_P] = (int) bitRead(BIAS_P, numCH-1); //set the array at BIAS_SENS (postive) routing state.
	channelSettings[numCH-1][CH_BIAS_ROUTING_N] = (int) bitRead(BIAS_N, numCH-1); //set the array at BIAS_SENS (negative) routing state.

	leadOffSettings[numCH-1][0] = leadOffSettings[numCH-1][1] = 0x00; // control ON/OFF impedance measurement.
	if(details){
		Serial.println("------------------------------------------------------");
	}
}

void BCI_LMSE_ESP::powerOnChannels(uint8_t numCH, uint8_t input_selection, int gain_selection){

	CHnSET0 = 0x00;
	//byte start_CH = 1;    // strating channels number. 
	//byte end_CH = 8;      // end channels number.
	SDATAC(); // used to stop RDATAC mode
	defaultChannelSettings(numCH);
		//Serial.println(channelSettings[numCH-1][CH_SRB2], BIN);
	switch(gain_selection){
		case PGA_GAIN1_SELECT:
			pga_gain_settings =(uint8_t) PGA_GAIN1_MASK; break;
		case PGA_GAIN2_SELECT:
			pga_gain_settings =(uint8_t) PGA_GAIN2_MASK; break;
		case PGA_GAIN4_SELECT:
			pga_gain_settings =(uint8_t) PGA_GAIN4_MASK; break;
		case PGA_GAIN6_SELECT:
			pga_gain_settings =(uint8_t) PGA_GAIN6_MASK; break;
		case PGA_GAIN8_SELECT:
			pga_gain_settings =(uint8_t) PGA_GAIN8_MASK; break;
		case PGA_GAIN12_SELECT:
			pga_gain_settings =(uint8_t) PGA_GAIN12_MASK; break;
		case PGA_GAIN24_SELECT:
			pga_gain_settings =(uint8_t) PGA_GAIN24_MASK;break;
		default:
			Serial.println("WRONG PGA GAIN VALUE ..");
			pga_gain_settings =(uint8_t) PGA_GAIN1_MASK; break;
			break;
	}
	channelSettings[numCH-1][CH_INPUT_SELECT] = (int) input_selection;
	
	if (input_selection == 2){ // if several channels are configured to be bias measurement inputs, 
								// ... then just a single channel (is the first) will be the input to measure the BIAS.
		//channelSettings[numCH-1][CH_INPUT_SELECT] = (int) input_selection;
		if (numCH-1 == 0){
			biasSignalMeasurements(numCH); 
			BIAS_SENS_routing[numCH-1] = false;
			channelSettings[numCH-1][BIAS_MEAS] = 1;
		}else{
			BIAS_SENS_routing[numCH-1] = true;
			channelSettings[numCH-1][BIAS_MEAS] = 0;
		}
	}else{

		BIAS_SENS_routing[numCH-1] = false;
		channelSettings[numCH-1][BIAS_MEAS] = 0;	
	}
	
	channelSettings[numCH-1][CH_GAIN] = (int) pga_gain_settings;

	CHnSET0 |=(channelSettings[numCH-1][CH_PWR_MODE]<<7); // to get Channel in power Mode ON (the bit 7).

	CHnSET0 |=(channelSettings[numCH-1][CH_GAIN]<<4); // to get gain from channelsettings array and put it in [4:6] bits.
	
	if(SRB2_include[numCH-1] == false) {
		channelSettings[numCH-1][CH_SRB2] = 0;
	}else{
		channelSettings[numCH-1][CH_SRB2] = 1;
	}

	CHnSET0 |=(channelSettings[numCH-1][CH_SRB2]<<3); // to get the state of the SRB2 connection and put it in its location


	CHnSET0 |=channelSettings[numCH-1][CH_INPUT_SELECT];
	WREG(CH1SET + (byte)(numCH-1), CHnSET0);
			if(test){
		Serial.println("");
		Serial.println(CHnSET0, BIN);
		Serial.println();
	}
	//CHnSET0 = 0x00; // reset CHnSET0 buffer. 

	// See datasheet page.59 for how to configure MISC1 register.
	if(SRB1_include == false) {   // the case where SRB1 is not included.
		channelSettings[numCH-1][CH_SRB1] = 0;
		MISC1_buff = RREG(MISC1); // Read the content of MISC1 into the buffer MISC1_buff.
		bitClear(MISC1_buff, 5); // Switch SRB1 is opened.
		WREG(MISC1, MISC1_buff); // write the new value of MISC1_buff to MISC1 register
		
	}else{					// the case where SRB1 will be connected to all 8 channels inverting inputs.
		channelSettings[numCH-1][CH_SRB1] = 1;
		MISC1_buff = RREG(MISC1); // Read the content of MISC1 into the buffer MISC1_buff.
		bitSet(MISC1_buff, 5); // Switch SRB1 is closed.
		WREG(MISC1, MISC1_buff); // write the new value of MISC1_buff to MISC1 register
	} 
  
	if(BIAS_SENS_routing[numCH-1] == false) {  		// Route Channel 'numCH' into BIAS AMP is disabled. 
		channelSettings[numCH-1][CH_BIAS_ROUTING_P] = 0;
		channelSettings[numCH-1][CH_BIAS_ROUTING_N] = 0;
		BIAS_P = RREG(BIAS_SENSP);
		BIAS_N = RREG(BIAS_SENSN);
		bitClear(BIAS_P, numCH-1);
		bitClear(BIAS_N, numCH-1);
		WREG(BIAS_SENSP, BIAS_P);
		WREG(BIAS_SENSN, BIAS_N);
	
	}else{  	// Route Channel 'numCH' into BIAS AMP is enabled. 
		channelSettings[numCH-1][CH_BIAS_ROUTING_P] = 1;
		channelSettings[numCH-1][CH_BIAS_ROUTING_N] = 1;
		BIAS_P = RREG(BIAS_SENSP);
		BIAS_N = RREG(BIAS_SENSN);
		bitSet(BIAS_P, numCH-1);
		bitSet(BIAS_N, numCH-1);
		WREG(BIAS_SENSP, BIAS_P);
		WREG(BIAS_SENSN, BIAS_N);
	}
		MISC1_buff = 0x00; // reset the MISC1_buff.
		BIAS_P = 0x00; // reset the CHnSET0 buffer.
		BIAS_N = 0x00; // reset the CHnSET1 buffer.
		pga_gain_settings = 0; // reset gain selection.
		//RDATAC();
		
}

boolean BCI_LMSE_ESP::testChannelActivity(uint8_t numCH){

	byte CHnSET0 = RREG(CH1SET + (byte)(numCH-1)); // subtracts 1 so that we're counting from 0.
	boolean CH_STATE = bitRead(CHnSET0, 7);
	if (details){
		Serial.print("Channel ");
		Serial.print(numCH);
		if (CH_STATE) { Serial.println(" is OFF"); }
		else {Serial.println(" is ON");}	
	}
	return CH_STATE;
}

void BCI_LMSE_ESP::zeroChannelSettings() {

	for(uint8_t i=0; i < 8; i++) {
		for(uint8_t j =0; j < 8; j++){
			channelSettings[i][j] = 0;
		}
	}
}

void BCI_LMSE_ESP::defaultChannelSettings(uint8_t numCH){
	channelSettings[numCH-1][CH_PWR_MODE] = 0;
	channelSettings[numCH-1][CH_GAIN] = 6;
	channelSettings[numCH-1][CH_SRB2] = 0;
	channelSettings[numCH-1][CH_INPUT_SELECT] = 0;
	channelSettings[numCH-1][CH_BIAS_ROUTING_P] = 0;
	channelSettings[numCH-1][CH_BIAS_ROUTING_N] = 0;
	channelSettings[numCH-1][BIAS_MEAS] = 0;
	channelSettings[numCH-1][CH_SRB1] = 0;
}
void BCI_LMSE_ESP::resetStatusRegister(){
	for(uint8_t i = 0; i < 3; i++){
		status_reg[i] = 0x00; 
	}

}
void BCI_LMSE_ESP::resetChannelsData(){
	for(uint8_t i=0; i < 8; i++){
		channel[i] = 0;
	}
}
void BCI_LMSE_ESP::mirrorInitialize(){
	for(byte i = 0x00; i <= 0x17; i++){
		mirror[i] = 0x00;
	}
}

void BCI_LMSE_ESP::setMirrorToDefault(){
	mirror[0]  = 0X3E;
	mirror[1]  = 0x96; 	mirror[2]  = 0xC0;  mirror[3]  = 0x60;   mirror[4]  = 0x00;   mirror[5]   = 0x61;
	mirror[6]  = 0x61; 	mirror[7]  = 0x61;  mirror[8]  = 0x61;   mirror[9]  = 0x61;   mirror[10]  = 0x61;
	mirror[11] = 0x61;	mirror[12] = 0x61;  mirror[13] = 0x00;   mirror[14] = 0x00;   mirror[15]  = 0x00;
	mirror[16] = 0x00; 	mirror[17] = 0x00;  mirror[18] = 0x00;   mirror[19] = 0x00;   mirror[20]  = 0x0F;
	mirror[21] = 0x00; 	mirror[22] = 0x00;  mirror[23] = 0x00; 
}

void BCI_LMSE_ESP::getMirrorContent(){
	for (uint8_t i=0; i<24; i++){
		Serial.print("Mirror[");
		Serial.print(i);
		Serial.print("]:");
		Serial.print(mirror[i], BIN);
		Serial.print(" (0x");
		Serial.print(mirror[i], HEX);
		Serial.print(")");
		if (i != 23){ Serial.print(" - "); }

	}
}

void BCI_LMSE_ESP::channelSettingsStats(void){
	Serial.print("      ");
	Serial.println("PWR_MODE - CH_GAIN - CH_SRB2 - CH_INPUT - BIAS_SENS_P - BIAS_SENS_N - BIAS_MEAS - CH_SRB1");
	for(uint8_t i=1; i<=number_CH; i++){
		Serial.print("CH0");
		Serial.print(i);
		Serial.print(":     ");
		for(uint8_t j=0; j< 8; j++){
			Serial.print(channelSettings[i-1][j]);
			Serial.print("         ");
			if(j==5) Serial.println();

		}
	}
}

/* ------------------------------------------------------*/
////////////////////**** Transfer Function *******************////////
byte  BCI_LMSE_ESP::SPI_transfer(byte _data) {
	byte ESP_byte = 0x00;
	ESP_byte = vspi-> transfer(_data);
   	SPI.endTransaction();
	 return ESP_byte;
}

//*********** printing HEX values ***************//
void BCI_LMSE_ESP::DecToHex(byte _regData){
	Serial.print("0x");
	if(_regData <0x10) {
		Serial.print("0");}
	Serial.print(_regData, HEX);
}
///////////// end ///////////////////
