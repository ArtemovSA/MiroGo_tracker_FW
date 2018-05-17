
#include "EXT_Flash.h"
#include "stdint.h"
#include "string.h"
#include "em_device.h"
#include "em_gpio.h"
#include "Delay.h"

uint8_t* SPI_RxBuffer;
int SPI_RxBufferSize;
volatile int SPI_RxBufferIndex;

uint8_t buf_flash[EXT_FLASH_IO_SIZE];

//���������
void SPI_sendBuffer(uint8_t* txBuffer, uint16_t bytesToSend); //Send data
void SPI_reciveBuffer(uint8_t* rxBuffer, uint16_t bytesToRecive); //Recive data
void EXT_Flash_write_enable(); //��������� ������
uint8_t EXT_Flash_waitStatusByte(uint8_t bit_num, uint8_t val, uint16_t wait_ms); //����� ����������� ���������� ����

//-------------------------------------------------------------------------------------------------------
//������������� Flash
uint8_t EXT_Flash_init() {

  EXT_FLASH_SPI_CS_SET;
  uint8_t EXT_flash_id = EXT_Flash_get_DevIDReg();   //������ DEV ID
  
  if (EXT_flash_id == 0x15)
    return 1;
  
  return 0;
}
//-------------------------------------------------------------------------------------------------------
//������ ��������� �������
void EXT_Flash_set_StatusReg(uint8_t value) {
  
  uint8_t address = EXT_FLASH_STATUS_SET;
  
  EXT_FLASH_SPI_CS_RESET;
  
  SPI_sendBuffer(&address,1);
  SPI_sendBuffer(&value, 1);
  
  EXT_FLASH_SPI_CS_SET;
}
//-------------------------------------------------------------------------------------------------------
//������� ������
void EXT_Flash_erace_all() {
  
  uint8_t command = EXT_FLASH_ERASE_CHIP;

  EXT_FLASH_SPI_CS_RESET;
  
  SPI_sendBuffer(&command,1);
  
  EXT_FLASH_SPI_CS_SET;
}
//-------------------------------------------------------------------------------------------------------
//������� ������ 4 ��
void EXT_Flash_erace_sector(uint32_t address) {
  
  uint8_t addr[3];
  uint8_t command = EXT_FLASH_ERASE_SECTOR;
  
  EXT_Flash_write_enable(); //��������� ������
  EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WEL, 1, 200); //���� ������� ������ ������
  
  addr[0] = (address & 0xFF0000) >> 16;
  addr[1] = (address & 0x00FF00) >> 8;
  addr[2] = (address & 0x0000FF);

  EXT_FLASH_SPI_CS_RESET;
  SPI_sendBuffer(&command,1);
  SPI_sendBuffer(addr,3);
  EXT_FLASH_SPI_CS_SET;
  _delay_ms(300);
  
  EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WIP, 0, 200); //���� ���������� ��������
}
//-------------------------------------------------------------------------------------------------------
//������� ���� 64 ��
void EXT_Flash_erace_block(uint32_t address) {
  
  uint8_t addr[3];
  uint8_t command = EXT_FLASH_ERASE_BLOCK;
  
  EXT_Flash_write_enable(); //��������� ������
  EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WEL, 1, 200); //���� ������� ������ ������
  
  addr[0] = (address & 0xFF0000) >> 16;
  addr[1] = (address & 0x00FF00) >> 8;
  addr[2] = (address & 0x0000FF);

  EXT_FLASH_SPI_CS_RESET;
  SPI_sendBuffer(&command,1);
  SPI_sendBuffer(addr,3);
  EXT_FLASH_SPI_CS_SET;
  _delay_ms(1000);
  
  EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WIP, 0, 5000); //���� ��������
}
//-------------------------------------------------------------------------------------------------------
//��������� ������
void EXT_Flash_write_enable() {
  
  uint8_t address = EXT_FLASH_WRITE_ENABLE;

  EXT_FLASH_SPI_CS_RESET;
  SPI_sendBuffer(&address,1);
  EXT_FLASH_SPI_CS_SET;
}

//-------------------------------------------------------------------------------------------------------
//������ ��������� �������
uint8_t EXT_Flash_get_StatusReg() {
  
  uint8_t value_flash;
  uint8_t address = EXT_FLASH_STATUS_GET;
  
  EXT_FLASH_SPI_CS_RESET;
  SPI_sendBuffer(&address,1);
  SPI_reciveBuffer(&value_flash, 1);
  EXT_FLASH_SPI_CS_SET;
  return value_flash;
}
//-------------------------------------------------------------------------------------------------------
//������ DEV ID
uint8_t EXT_Flash_get_DevIDReg() {
  
  uint8_t value_flash;
  uint8_t address = EXT_FLASH_DEV_ID_GET;
  uint8_t dummy[3];
  
  dummy[0] = 0x00;
  dummy[1] = 0x00;
  dummy[2] = 0x00;
  
  EXT_FLASH_SPI_CS_RESET;
  
  SPI_sendBuffer(&address,1);
  SPI_sendBuffer(dummy,3);
  SPI_reciveBuffer(&value_flash, 1);
  
  EXT_FLASH_SPI_CS_SET;
  
  return value_flash;
}
//-------------------------------------------------------------------------------------------------------
//���������� ������ ������ � �������
void EXT_Flash_ReWriteData(uint32_t address, uint8_t *data, uint16_t count) {

  uint32_t sector_addr = (uint32_t)(address/EXT_FLASH_SECTOR_SIZE)*EXT_FLASH_SECTOR_SIZE; //����� ����� �������
  uint16_t len = 0;
  uint32_t part_addr = sector_addr;
   
  memset(buf_flash,0,sizeof(buf_flash)); //�������� �����
  
  EXT_Flash_erace_sector(EXT_FLASH_TEMP_SECTOR); //������� �������� ������
  _delay_ms(10);
  
  //�� ������ ��������� ������ �� ������, �������� � �������� � �������� ������
  for (uint16_t part_n = 0; part_n<EXT_FLASH_SECTOR_SIZE/EXT_FLASH_IO_SIZE; part_n++)
  {
    part_addr = sector_addr+part_n*EXT_FLASH_IO_SIZE;
    EXT_Flash_readData(part_addr, buf_flash,EXT_FLASH_IO_SIZE); //������ ������
    
    //���� ������ ��������� ������ ����� � ��� ��������� �����
    if ((address <= part_addr+EXT_FLASH_IO_SIZE)&(count > 0))
    {
      //������ ����� ������
      if (count > EXT_FLASH_IO_SIZE)
        len = part_addr+EXT_FLASH_IO_SIZE-address;
      else
        len = count;
      
     //������ ������
     if (len > EXT_FLASH_IO_SIZE)
        len = EXT_FLASH_IO_SIZE;
      
      memcpy(&buf_flash[address-part_addr],data,len); //���������� ������

      data += len;
      address += len;
      count -= len;
    }
    
    //�������� ����� � �������� ������
    EXT_Flash_writeData(EXT_FLASH_TEMP_SECTOR+part_n*EXT_FLASH_IO_SIZE, buf_flash,EXT_FLASH_IO_SIZE);
  }
  
  //������� ������� ������
  EXT_Flash_erace_sector(sector_addr);
  _delay_ms(10);
  
  //���������� �������� ������ �������
  //�� ������ ��������� ������ �� ������, �������� � �������� � �������� ������
  for (uint16_t part_n = 0; part_n<EXT_FLASH_SECTOR_SIZE/EXT_FLASH_IO_SIZE; part_n++)
  {
    EXT_Flash_readData(EXT_FLASH_TEMP_SECTOR+part_n*EXT_FLASH_IO_SIZE, buf_flash, EXT_FLASH_IO_SIZE);
    EXT_Flash_writeData(sector_addr+part_n*EXT_FLASH_IO_SIZE, buf_flash,EXT_FLASH_IO_SIZE);
  }
}
//-------------------------------------------------------------------------------------------------------
//����� ����������� ���������� ����
uint8_t EXT_Flash_waitStatusByte(uint8_t bit_num, uint8_t val, uint16_t wait_ms) {
  uint16_t try_counter = 0;
  while ((EXT_Flash_get_StatusReg() & (1<<bit_num)) != (val<<bit_num)) {
    if (try_counter == wait_ms)
      return 0;
   _delay_ms(1);
   try_counter++;
  }
  return 1;
}
//-------------------------------------------------------------------------------------------------------
//������ ������ ������
void EXT_Flash_writeData(uint32_t address, uint8_t *data, uint16_t count) {
  
  uint8_t addr[3];
  uint8_t command = EXT_FLASH_WRITE_DATA;
  uint8_t count_full = (uint32_t)(count/EXT_FLASH_IO_SIZE); //����� ���������� �������
  uint32_t buf_addr = address;
  uint16_t count_end;
  
  //���������� ����� ��������
  for (int i=0; i<count_full; i++) {
    
    EXT_Flash_write_enable(); //��������� ������
    EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WEL, 1, 200); //���� ����� ������ ������
    
    addr[0] = (buf_addr & 0xFF0000) >> 16;
    addr[1] = (buf_addr & 0x00FF00) >> 8;
    addr[2] = (buf_addr & 0x0000FF);
    
    memcpy(buf_flash,&data[EXT_FLASH_IO_SIZE*i],EXT_FLASH_IO_SIZE);
    
    EXT_FLASH_SPI_CS_RESET;
    SPI_sendBuffer(&command, 1);
    SPI_sendBuffer(addr, 3);
    SPI_sendBuffer(buf_flash, EXT_FLASH_IO_SIZE);
    EXT_FLASH_SPI_CS_SET;
    _delay_ms(5);
    
    EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WIP, 0, 200); //���� ���������� ������
    
    buf_addr += EXT_FLASH_IO_SIZE;
  }
  
  //������� ���������� ��������� ����
  count_end = count-(count_full*EXT_FLASH_IO_SIZE);
  
  //���������� ��������� ������
  if (count_end > 0) {
    
    EXT_Flash_write_enable(); //��������� ������
    EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WEL, 1, 200); //���� ����� ������ ������
    
    addr[0] = (buf_addr & 0xFF0000) >> 16;
    addr[1] = (buf_addr & 0x00FF00) >> 8;
    addr[2] = (buf_addr & 0x0000FF);
    
    memcpy(buf_flash,&data[EXT_FLASH_IO_SIZE*count_full],count_end);
    
    EXT_FLASH_SPI_CS_RESET;
    SPI_sendBuffer(&command, 1);
    SPI_sendBuffer(addr, 3);
    SPI_sendBuffer(buf_flash, count_end);
    EXT_FLASH_SPI_CS_SET;
    _delay_ms(5);
    
    EXT_Flash_waitStatusByte(EXT_FLASH_STATUS_WIP, 0, 200); //���� ���������� ������
  }  
  
}
//-------------------------------------------------------------------------------------------------------
//������ ������ ������
void EXT_Flash_readData(uint32_t address, uint8_t *data, uint16_t count) {
  
  uint8_t addr[3];
  uint8_t command = EXT_FLASH_READ_DATA;
  uint8_t count_full = (uint32_t)(count/EXT_FLASH_IO_SIZE); //����� ���������� �������
  uint32_t buf_addr = address;
    
  //������ ����� ������
  for (int i=0; i<count_full; i++) {
  
    addr[0] = (buf_addr & 0xFF0000) >> 16;
    addr[1] = (buf_addr & 0x00FF00) >> 8;
    addr[2] = (buf_addr & 0x0000FF);
    
    EXT_FLASH_SPI_CS_RESET;
    
    SPI_sendBuffer(&command, 1);
    SPI_sendBuffer(addr, 3);
    SPI_reciveBuffer(&data[EXT_FLASH_IO_SIZE*i], EXT_FLASH_IO_SIZE);
    
    EXT_FLASH_SPI_CS_SET;
    
    buf_addr += EXT_FLASH_IO_SIZE;
  }
  
  //�������� ��������� ������
  if (count-(count_full*EXT_FLASH_IO_SIZE) > 0) {
    
    addr[0] = (buf_addr & 0xFF0000) >> 16;
    addr[1] = (buf_addr & 0x00FF00) >> 8;
    addr[2] = (buf_addr & 0x0000FF);
    
    EXT_FLASH_SPI_CS_RESET;
    
    SPI_sendBuffer(&command, 1);
    SPI_sendBuffer(addr, 3);
    SPI_reciveBuffer(&data[EXT_FLASH_IO_SIZE*count_full], count-(count_full*EXT_FLASH_IO_SIZE));
    
    EXT_FLASH_SPI_CS_SET;
  }
}
//-------------------------------------------------------------------------------------------------------
//Recive data
void SPI_reciveBuffer(uint8_t* rxBuffer, uint16_t bytesToRecive)
{
  int ii;

  SPI_RxBuffer      = rxBuffer;
  SPI_RxBufferSize  = bytesToRecive;
  SPI_RxBufferIndex = 0;
  
  USART0->CMD = USART_CMD_CLEARRX;

  /* Enable interrupts */
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);
  NVIC_EnableIRQ(USART0_RX_IRQn);
  
  USART0->IEN |= USART_IEN_RXDATAV;
  
  for (ii = 0; ii < bytesToRecive;  ii++)
  {
    while (!(USART0->STATUS & USART_STATUS_TXBL));
    USART0->TXDATA = 0;
  }

  while (!(USART0->STATUS & USART_STATUS_TXC));

}
//-------------------------------------------------------------------------------------------------------
//Send data
void SPI_sendBuffer(uint8_t* txBuffer, uint16_t bytesToSend)
{
  /* Sending the data */
  for (int ii = 0; ii < bytesToSend;  ii++)
  {
    /* Waiting for the usart to be ready */
    while (!(USART0->STATUS & USART_STATUS_TXBL)) ;

    if (txBuffer != 0)
    {
      /* Writing next byte to USART */
      USART0->TXDATA = *txBuffer;
      txBuffer++;
    }
    else
    {
      USART0->TXDATA = 0;
    }
  }

  /*Waiting for transmission of last byte */
  while (!(USART0->STATUS & USART_STATUS_TXC)) ;
}
//-------------------------------------------------------------------------------------------------------
//RX IRQ
void USART0_RX_IRQHandler(void)
{
  uint8_t rxdata;

  if (USART0->STATUS & USART_STATUS_RXDATAV)
  {
    /* Reading out data */
    rxdata = USART0->RXDATA;

    if (SPI_RxBufferIndex < SPI_RxBufferSize)
    {
      /* Store Data */
      SPI_RxBuffer[SPI_RxBufferIndex] = rxdata;
      SPI_RxBufferIndex++;
    }
  }
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);
}