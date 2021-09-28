#include "bootloader.h"
#include "main.h"
#include "stdio.h"
/**
 * @bieaf 擦除页
 *
 * @param pageaddr  起始地址	
 * @param num       擦除的页数
 * @return 1
 */
static int Erase_page(uint32_t pageaddr, uint32_t num)
{
        int ret = HAL_ERROR;
        uint32_t PageError_cnt = 0;
        uint32_t PageError = 0;
	HAL_FLASH_Unlock();
	
	/* 擦除FLASH*/
	FLASH_EraseInitTypeDef FlashSet;
	FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashSet.PageAddress = pageaddr;
	FlashSet.NbPages = num;
	
	/*设置PageError，调用擦除函数*/
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
 * @bieaf 写若干个数据
 *
 * @param addr       写入的地址
 * @param buff       写入数据的起始地址
 * @param word_size  长度
 * @return 
 */
static int WriteFlash(uint32_t addr, uint32_t * buff, int word_size)
{
#if 1
        int ret = HAL_ERROR;
	/* 1/4解锁FLASH*/
	HAL_FLASH_Unlock();
	
	for(int i = 0; i < word_size; i++)	
	{		/* 3/4对FLASH烧写*/

           ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + 4 * i, buff[i]);
           if(ret == HAL_ERROR)
           {
               return ret;
           }

	}

	/* 4/4锁住FLASH*/
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
 * @bieaf 读若干个数据
 *
 * @param addr       读数据的地址
 * @param buff       读出数据的数组指针
 * @param word_size  长度
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


/* 读取启动模式 */
unsigned int Read_Start_Mode(void)
{
	unsigned int mode = 0;
	ReadFlash((Application_2_Addr + Application_Size - 4), &mode, 1);
	return mode;
}



/**
 * @bieaf 进行程序的覆盖
 * @detail 1.擦除目的地址
 *         2.源地址的代码拷贝到目的地址
 *         3.擦除源地址
 *
 * @param  搬运的源地址
 * @param  搬运的目的地址
 * @return 搬运的程序大小
 */
void MoveCode(unsigned int src_addr, unsigned int des_addr, unsigned int byte_size)
{
	/*1.擦除目的地址*/
	//usb_printf("> Start erase des flash......\r\n");
	Erase_page(des_addr, (byte_size/PageSize));
	//usb_printf("> Erase des flash down......\r\n");
	
	/*2.开始拷贝*/	
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
	
	/*3.擦除源地址*/
	//usb_printf("> Start erase src flash......\r\n");
	Erase_page(src_addr, (byte_size/PageSize));
	//usb_printf("> Erase src flash down......\r\n");
}



/* 采用汇编设置栈的值 */
void MSR_MSP (uint32_t ulAddr) 
{
    asm("MSR MSP, r0"); //set Main Stack value
    asm("BX r14");
}



/* 程序跳转函数 */
typedef void (*Jump_Fun)(void);
void IAP_ExecuteApp (uint32_t App_Addr)
{
    Jump_Fun JumpToApp;  
    if ( ( ( * ( __IO uint32_t * ) App_Addr ) & 0x2FFE0000 ) == 0x20000000 )	//检查栈顶地址是否合法.
    { 
	JumpToApp = (Jump_Fun) * ( __IO uint32_t *)(App_Addr + 4);		//用户代码区第二个字为程序开始地址(复位地址)		
	MSR_MSP( * ( __IO uint32_t * ) App_Addr );				//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
        for(int i = 0; i < 8; i++)
        {			
            NVIC->ICER[i] = 0xFFFFFFFF;	/* 关闭中断*/
            NVIC->ICPR[i] = 0xFFFFFFFF;	/* 清除中断标志位 */
	}
	JumpToApp();								//跳转到APP.
    }
}



/**
 * @bieaf 进行BootLoader的启动
 *
 * @param none
 * @return none
 */
void Start_BootLoader(void)
{
	/*==========打印消息==========*/  	
	 //usb_printf("\r\n");
	 //usb_printf("***********************************\r\n");
	 //usb_printf("*                                 *\r\n");       
	 //usb_printf("*    Cabinet's BootLoader      *\r\n");       
	 //usb_printf("*                                 *\r\n");       
	 //usb_printf("***********************************\r\n");
	
	//usb_printf("> Choose a startup method......\r\n");	
	switch(Read_Start_Mode())									///< 读取是否启动应用程序 */
	{
		case Startup_Normol:										///< 正常启动 */
		{
			//usb_printf("> Normal start......\r\n");
			break;
		}
		case Startup_Update:										///< 升级再启动 */
		{
			//usb_printf("> Start update......\r\n");		
			MoveCode(Application_2_Addr, Application_1_Addr, Application_Size);
			//usb_printf("> Update down......\r\n");
			break;
		}
		case Startup_Reset:										///< 恢复出厂设置 目前没使用 */
		{
			//usb_printf("> Restore to factory program......\r\n");
			break;			
		}
		default:														///< 启动失败
		{
			//usb_printf("> Error:%X!!!......\r\n", Read_Start_Mode());
			return;			
		}
	}
	
	/* 跳转到应用程序 */
	//usb_printf("> Start up......\r\n\r\n");	
	IAP_ExecuteApp(Application_1_Addr);
}


