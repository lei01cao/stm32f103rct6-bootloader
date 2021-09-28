#ifndef __BOOTLOADER_H_
#define __BOOTLOADER_H_

#define PageSize		FLASH_PAGE_SIZE			//2K

/*=====�û�����(�����Լ��ķ�����������)=====*/
#define BootLoader_Size 		0x5800U			///< BootLoader�Ĵ�С 22K
#define Application_Size		0x14000U	        ///< Ӧ�ó���Ĵ�С 80K

#define Application_1_Addr		0x08005800U		///< Ӧ�ó���1���׵�ַ
#define Application_2_Addr		0x08019800U		///< Ӧ�ó���2���׵�ַ
/*==========================================*/



/* �����Ĳ��� */
#define Startup_Normol 0xFFFFFFFF	///< ��������
#define Startup_Update 0xAAAAAAAA	///< ����������
#define Startup_Reset  0x5555AAAA	///< ***�ָ����� Ŀǰûʹ��***



void Start_BootLoader(void);




#endif
