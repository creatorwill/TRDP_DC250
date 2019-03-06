#include "stdafx.h"
#include "TRDPServer.h"
#include "TRDPServerDlg.h"
#include "sharememoryclient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_SERVERDATACHANGE WM_USER + 999
#define WM_CLIENTDATACNANGE WM_USER + 998

HANDLE hMapFile_mvb2trdp;
HANDLE hMapFile_trdp2mvb;

Csharememoryclient::Csharememoryclient()
{
}

Csharememoryclient::~Csharememoryclient()
{
}

/**
 * @brief    -- initsharememory  共享内存初始化， 打开映射。
 * @return   -- 
 */
UINT Csharememoryclient:: initsharememory()
{
	/* TRDP && MVB */
	hMapFile_trdp2mvb = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(st_shared_memory), "NewMemoryTrdpAndMvb");
	if (!hMapFile_trdp2mvb) {
		AfxMessageBox(TEXT("创建映射对象失败"));
		return 1;
	}

	DWORD dwMapErrTrdp = GetLastError();
	memory_trdp_mvb = (st_shared_memory*)MapViewOfFile(hMapFile_trdp2mvb, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (NULL == memory_trdp_mvb) {
		CloseHandle(hMapFile_trdp2mvb);
		hMapFile_trdp2mvb = NULL;
		AfxMessageBox("读取文件映射失败");
		return 1;
	}

	for (int i = 0; i < 6; i++) {
		memory_trdp_mvb->isValid[i] = false;
	}

	if (dwMapErrTrdp == ERROR_ALREADY_EXISTS) { // 如果该共享内存已经存在
		memory_trdp_mvb->bFlag = true;

	} else {
		memory_trdp_mvb->bFlag = false;
	}

    return 0;
}

/**
 * @brief    -- WriteTrdpDatatoMvb 把TRDP数据写入共享内存，供MVB调用使用。
 * @param[i] -- buf                预存放的TRDP数据
 * @param[i] -- index              共享内存结构体数组索引值。
 *                                 0  "comm-record" : 公共端口:   0~9999，(端口:1701,1702), 记录内容: 10000~19999, (端口:1801,1802,1301,1302);
 *                                 1  "tcn-bcn"     : 过程数据:   0~19999 
 *                                 2  "fault1"      : 一二级故障: c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *                                 3  "fault3"      : 三级故障:   c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *                                 4  "other"       : 编号 预留
 *                                 5  "mvb"         : 存放 mvb数据 , 供 trdp进程 读取
 * @return   -- 
 */
Csharememoryclient:: WriteTrdpDatatoMvb(st_data_info *buf, unsigned index)
{
	try {
		memcpy(memory_trdp_mvb->memory_data[index].strType, buf->strType, strlen(buf->strType));
		memcpy(memory_trdp_mvb->memory_data[index].data, buf->data, 20000);
		memory_trdp_mvb->isValid[index] = true;
	} catch (...) {
		printf("Write shared memory failed");
		// AfxMessageBox("Write shared memory failed");
	}
}

/**
 * @brief    -- ReadMvbDatatoTrdp  : 读取共享内存中的数据
 * @param[o] -- buf                : 把数据读入该存储空间
 * @param[i] -- index              : 索引,
 */
Csharememoryclient::ReadMvbDatatoTrdp(st_data_info *buf, unsigned index)
{
	try {
		memcpy(buf->strType, memory_trdp_mvb->memory_data[index].strType, strlen(memory_trdp_mvb->memory_data[index].strType));
		// printf("Read shared memory[%d] vald: %d \r\n" , index, memory_trdp_mvb->isValid[index]);


		for (int j = 0; j < 20000; j++) {
			buf->data[j] = memory_trdp_mvb->memory_data[index].data[j];	
		}
	} catch (...) {
		printf("Read shared memory failed");
		// AfxMessageBox("Read shared memory failed");
	}
}
