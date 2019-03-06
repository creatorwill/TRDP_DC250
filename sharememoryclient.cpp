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
 * @brief    -- initsharememory  �����ڴ��ʼ���� ��ӳ�䡣
 * @return   -- 
 */
UINT Csharememoryclient:: initsharememory()
{
	/* TRDP && MVB */
	hMapFile_trdp2mvb = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(st_shared_memory), "NewMemoryTrdpAndMvb");
	if (!hMapFile_trdp2mvb) {
		AfxMessageBox(TEXT("����ӳ�����ʧ��"));
		return 1;
	}

	DWORD dwMapErrTrdp = GetLastError();
	memory_trdp_mvb = (st_shared_memory*)MapViewOfFile(hMapFile_trdp2mvb, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (NULL == memory_trdp_mvb) {
		CloseHandle(hMapFile_trdp2mvb);
		hMapFile_trdp2mvb = NULL;
		AfxMessageBox("��ȡ�ļ�ӳ��ʧ��");
		return 1;
	}

	for (int i = 0; i < 6; i++) {
		memory_trdp_mvb->isValid[i] = false;
	}

	if (dwMapErrTrdp == ERROR_ALREADY_EXISTS) { // ����ù����ڴ��Ѿ�����
		memory_trdp_mvb->bFlag = true;

	} else {
		memory_trdp_mvb->bFlag = false;
	}

    return 0;
}

/**
 * @brief    -- WriteTrdpDatatoMvb ��TRDP����д�빲���ڴ棬��MVB����ʹ�á�
 * @param[i] -- buf                Ԥ��ŵ�TRDP����
 * @param[i] -- index              �����ڴ�ṹ����������ֵ��
 *                                 0  "comm-record" : �����˿�:   0~9999��(�˿�:1701,1702), ��¼����: 10000~19999, (�˿�:1801,1802,1301,1302);
 *                                 1  "tcn-bcn"     : ��������:   0~19999 
 *                                 2  "fault1"      : һ��������: c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *                                 3  "fault3"      : ��������:   c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *                                 4  "other"       : ��� Ԥ��
 *                                 5  "mvb"         : ��� mvb���� , �� trdp���� ��ȡ
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
 * @brief    -- ReadMvbDatatoTrdp  : ��ȡ�����ڴ��е�����
 * @param[o] -- buf                : �����ݶ���ô洢�ռ�
 * @param[i] -- index              : ����,
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
