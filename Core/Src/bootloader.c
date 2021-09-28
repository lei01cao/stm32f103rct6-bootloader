#include "bootloader.h"
#include "main.h"
#include "stdio.h"
/**
 * @bieaf ����ҳ
 *
 * @param pageaddr  ��ʼ��ַ	
 * @param num       ������ҳ��
 * @return 1
 */
static int Erase_page(uint32_t pageaddr, uint32_t num)
{
        int ret = HAL_ERROR;
        uint32_t PageError_cnt = 0;
        uint32_t PageError = 0;
	HAL_FLASH_Unlock();
	
	/* ����FLASH*/
	FLASH_EraseInitTypeDef FlashSet;
	FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashSet.PageAddress = pageaddr;
	FlashSet.NbPages = num;
	
	/*����PageError�����ò�������*/
        while(ret == HAL_ERROR)
        {
          PageError_cnt++;
          if(PageError_cnt > 10)
          {
            return HAL_ERROR;
          }
          ret = HAL_FLASHEx_Erase(&FlashSet, &PageError); 
          HAL_Delay(50);
        }
	//uint32_t PageError = 0;
	//HAL_FLASHEx_Erase(&FlashSet, &PageError);
	
	HAL_FLASH_Lock();
	return 1;
}


/**
 * @bieaf д���ɸ�����
 *
 * @param addr       д��ĵ�ַ
 * @param buff       д�����ݵ���ʼ��ַ
 * @param word_size  ����
 * @return 
 */
static int WriteFlash(uint32_t addr, uint32_t * buff, int word_size)
{
#if 1
        int ret = HAL_ERROR;
	/* 1/4����FLASH*/
	HAL_FLASH_Unlock();
	
	for(int i = 0; i < word_size; i++)	
	{		/* 3/4��FLASH��д*/

           ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + 4 * i, buff[i]);
           if(ret == HAL_ERROR)
           {
               return ret;
           }

	}

	/* 4/4��סFLASH*/
	HAL_FLASH_Lock();
        return ret;
#else      
    int ret = HAL_ERROR;
    HAL_FLASH_Unlock();
    uint32_t temp = 0;
    for(int i = 0; i < word_size; i++)
    {
      temp = *(buff+i);
      ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD , addr+i*4, temp);  
      if(ret == HAL_ERROR)
      {
        return ret;
      }
    }
    HAL_FLASH_Lock(); 
    return HAL_OK;
#endif
}



/**
 * @bieaf �����ɸ�����
 *
 * @param addr       �����ݵĵ�ַ
 * @param buff       �������ݵ�����ָ��
 * @param word_size  ����
 * @return 
 */
static void ReadFlash(uint32_t addr, uint32_t * buff, uint16_t word_size)
{
    for(int i =0; i < word_size; i++)
    {
	buff[i] = *(__IO uint32_t*)(addr + 4 * i);
    }
    return;
}


/* ��ȡ����ģʽ */
unsigned int Read_Start_Mode(void)
{
	unsigned int mode = 0;
	ReadFlash((Application_2_Addr + Application_Size - 4), &mode, 1);
	return mode;
}



/**
 * @bieaf ���г���ĸ���
 * @detail 1.����Ŀ�ĵ�ַ
 *         2.Դ��ַ�Ĵ��뿽����Ŀ�ĵ�ַ
 *         3.����Դ��ַ
 *
 * @param  ���˵�Դ��ַ
 * @param  ���˵�Ŀ�ĵ�ַ
 * @return ���˵ĳ����С
 */
void MoveCode(unsigned int src_addr, unsigned int des_addr, unsigned int byte_size)
{
	/*1.����Ŀ�ĵ�ַ*/
	//usb_printf("> Start erase des flash......\r\n");
	Erase_page(des_addr, (byte_size/PageSize));
	//usb_printf("> Erase des flash down......\r\n");
	
	/*2.��ʼ����*/	
	unsigned int temp_app2[256];
        unsigned int temp_app1[256];
	int error_cnt;
	//usb_printf("> Start copy......\r\n");
	for(int i = 0; i < byte_size/1024; i++)
	{
          while(1)
          {
            error_cnt = 0;
            ReadFlash((src_addr + i*1024), temp_app2, 256);
            WriteFlash((des_addr + i*1024), temp_app2, 256);
            ReadFlash((des_addr + i*1024), temp_app1, 256);
            for(int j = 0; j < 256; j++)
            {
              if(temp_app1[j] == temp_app2[j])
              {
                error_cnt++;
              }
            }
            if(error_cnt == 256)
            {
              break;
            }
            HAL_Delay(2);
          }
	}
	//usb_printf("> Copy down......\r\n");
	
	/*3.����Դ��ַ*/
	//usb_printf("> Start erase src flash......\r\n");
	Erase_page(src_addr, (byte_size/PageSize));
	//usb_printf("> Erase src flash down......\r\n");
}



/* ���û������ջ��ֵ */
void MSR_MSP (uint32_t ulAddr) 
{
    asm("MSR MSP, r0"); //set Main Stack value
    asm("BX r14");
}



/* ������ת���� */
typedef void (*Jump_Fun)(void);
void IAP_ExecuteApp (uint32_t App_Addr)
{
    Jump_Fun JumpToApp;  
    if ( ( ( * ( __IO uint32_t * ) App_Addr ) & 0x2FFE0000 ) == 0x20000000 )	//���ջ����ַ�Ƿ�Ϸ�.
    { 
	JumpToApp = (Jump_Fun) * ( __IO uint32_t *)(App_Addr + 4);		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
	MSR_MSP( * ( __IO uint32_t * ) App_Addr );				//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
        for(int i = 0; i < 8; i++)
        {			
            NVIC->ICER[i] = 0xFFFFFFFF;	/* �ر��ж�*/
            NVIC->ICPR[i] = 0xFFFFFFFF;	/* ����жϱ�־λ */
	}
	JumpToApp();								//��ת��APP.
    }
}



/**
 * @bieaf ����BootLoader������
 *
 * @param none
 * @return none
 */
void Start_BootLoader(void)
{
	/*==========��ӡ��Ϣ==========*/  	
	 //usb_printf("\r\n");
	 //usb_printf("***********************************\r\n");
	 //usb_printf("*                                 *\r\n");       
	 //usb_printf("*    Cabinet's BootLoader      *\r\n");       
	 //usb_printf("*                                 *\r\n");       
	 //usb_printf("***********************************\r\n");
	
	//usb_printf("> Choose a startup method......\r\n");	
	switch(Read_Start_Mode())									///< ��ȡ�Ƿ�����Ӧ�ó��� */
	{
		case Startup_Normol:										///< �������� */
		{
			//usb_printf("> Normal start......\r\n");
			break;
		}
		case Startup_Update:										///< ���������� */
		{
			//usb_printf("> Start update......\r\n");		
			MoveCode(Application_2_Addr, Application_1_Addr, Application_Size);
			//usb_printf("> Update down......\r\n");
			break;
		}
		case Startup_Reset:										///< �ָ��������� Ŀǰûʹ�� */
		{
			//usb_printf("> Restore to factory program......\r\n");
			break;			
		}
		default:														///< ����ʧ��
		{
			//usb_printf("> Error:%X!!!......\r\n", Read_Start_Mode());
			return;			
		}
	}
	
	/* ��ת��Ӧ�ó��� */
	//usb_printf("> Start up......\r\n\r\n");	
	IAP_ExecuteApp(Application_1_Addr);
}


